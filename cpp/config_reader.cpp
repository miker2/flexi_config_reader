#include "config_reader.h"

#include <fmt/format.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <range/v3/all.hpp>  // get everything (consider pruning this down a bit)
#include <regex>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/trace.hpp>

#include "config_actions.h"
#include "config_classes.h"
#include "config_exceptions.h"
#include "config_grammar.h"
#include "config_helpers.h"
#include "logger.h"
#include "utils.h"

auto ConfigReader::parse(const std::filesystem::path& cfg_filename) -> bool {
  bool success = true;
  peg::file_input cfg_file(cfg_filename);
  try {
    success = peg::parse<config::grammar, config::action>(cfg_file, out_);
    // If parsing is successful, all of these containers should be emtpy (consumed into
    // 'out_.cfg_res').
    success &= out_.keys.empty();
    success &= out_.flat_keys.empty();
    success &= out_.objects.empty();
    success &= out_.obj_res == nullptr;

    // Eliminate any vector elements with an empty map.
    out_.cfg_res |=
        ranges::actions::remove_if([](const config::types::CfgMap& m) { return m.empty(); });

    if (!success) {
      logger::critical("  Parse failure");
      logger::error("  cfg_res size: {}", out_.cfg_res.size());

      logger::error("Incomplete output: \n");
      out_.print();

      // Print a trace if a failure occured.
      cfg_file.restart();
      peg::standard_trace<config::grammar>(cfg_file);
      return success;
    }
  } catch (const peg::parse_error& e) {
    success = false;
    logger::critical("!!!");
    logger::critical("  Parser failure!");
    const auto p = e.positions().front();
    logger::critical("{}", e.what());
    logger::critical("{}", cfg_file.line_at(p));
    logger::critical("{}^", std::string(' ', p.column - 1));
    out_.print();
    logger::critical("!!!");
    return success;
  }

  config::types::CfgMap flat{};
  for (const auto& e : out_.cfg_res) {
    flat = flattenAndFindProtos(e, "", flat);
  }
  logger::debug("Flattened: \n{}", fmt::join(flat, "\n"));

  logger::debug("Protos: \n  {}", fmt::join(protos_ | ranges::views::keys, "\n  "));

  cfg_data_ = mergeNested(out_.cfg_res);

  const std::string debug_sep(35, '=');
#if 1
  // TODO: Determine if this is actually necessary.
  logger::trace("{} Strip Protos {}", debug_sep, debug_sep);
  stripProtos(cfg_data_);
  logger::trace(" --- Result of 'stripProtos':\n{}", fmt::join(cfg_data_, "\n"));
#endif

  logger::trace("{} Resolving References {}", debug_sep, debug_sep);
  resolveReferences(cfg_data_, "", {});
  logger::trace("\n{}", fmt::join(cfg_data_, "\n"));

  // Unflatten any flat keys:
  const auto flat_keys =
      cfg_data_ | ranges::views::keys |
      ranges::views::filter([](auto& key) { return key.find(".") != std::string::npos; }) |
      ranges::to<std::vector<std::string>> | ranges::actions::sort | ranges::actions::reverse;
  logger::debug("The following keys need to be flattened: {}", flat_keys);
  for (const auto& key : flat_keys) {
    config::helpers::unflatten(key, cfg_data_);
  }

  resolveVarRefs(cfg_data_, cfg_data_);

  // This isn't entirely necessary, but it cleans up the tree.
  config::helpers::removeEmpty(cfg_data_);

  return success;
}

void ConfigReader::dump() const { std::cout << cfg_data_; }

void ConfigReader::convert(const std::string& value_str, float& value) const {
  std::size_t len{0};
  value = std::stof(value_str, &len);
  if (len != value_str.size()) {
    throw config::MismatchTypeException(
        fmt::format("Error while converting '{}' to type float. Processed {} of {} characters",
                    value_str, len, value_str.size()));
  }
}

void ConfigReader::convert(const std::string& value_str, double& value) const {
  std::size_t len{0};
  value = std::stod(value_str, &len);
  if (len != value_str.size()) {
    throw config::MismatchTypeException(
        fmt::format("Error while converting '{}' to type double. Processed {} of {} characters",
                    value_str, len, value_str.size()));
  }
}

void ConfigReader::convert(const std::string& value_str, int& value) const {
  std::size_t len{0};
  value = std::stoi(value_str, &len);
  if (len != value_str.size()) {
    throw config::MismatchTypeException(
        fmt::format("Error while converting '{}' to type int. Processed {} of {} characters",
                    value_str, len, value_str.size()));
  }
}

void ConfigReader::convert(const std::string& value_str, int64_t& value) const {
  std::size_t len{0};
  value = std::stoll(value_str, &len);
  if (len != value_str.size()) {
    throw config::MismatchTypeException(
        fmt::format("Error while converting '{}' to type int64_t. Processed {} of {} characters",
                    value_str, len, value_str.size()));
  }
}

void ConfigReader::convert(const std::string& value_str, bool& value) const {
  std::size_t len{0};
  value = static_cast<bool>(std::stoi(value_str, &len));
  if (len != value_str.size()) {
    throw config::MismatchTypeException(
        fmt::format("Error while converting '{}' to type bool. Processed {} of {} characters",
                    value_str, len, value_str.size()));
  }
}

void ConfigReader::convert(const std::string& value_str, std::string& value) const {
  value = value_str;
}

auto ConfigReader::flattenAndFindProtos(const config::types::CfgMap& in,
                                        const std::string& base_name,
                                        config::types::CfgMap flattened) -> config::types::CfgMap {
  for (const auto& e : in) {
    const auto new_name = utils::join({base_name, e.first}, ".");
    const auto struct_like = dynamic_pointer_cast<config::types::ConfigStructLike>(e.second);
    if (struct_like != nullptr) {
      if (struct_like->type == config::types::Type::kProto) {
        protos_[new_name] = dynamic_pointer_cast<config::types::ConfigProto>(struct_like);
      }
      flattened = flattenAndFindProtos(struct_like->data, new_name, flattened);
    } else {
      flattened[new_name] = e.second;
    }
  }
  return flattened;
}

auto ConfigReader::mergeNested(const std::vector<config::types::CfgMap>& in)
    -> config::types::CfgMap {
  if (in.empty()) {
    // TODO: Throw exception here? How to handle empty vector?
    return {};
  }

  // This whole function is quite inefficient, but it works, which is a start:

  // Start with the first element:
  config::types::CfgMap squashed_cfg = in[0];

  // Accumulate all of the other elements of the vector into the CfgMap.
  for (auto& cfg : ranges::views::tail(in)) {
    logger::trace(
        "======= Working on merging: =======\n{}\n"
        "++++++++++++++ and ++++++++++++++++\n{}",
        fmt::join(squashed_cfg, "\n"), fmt::join(cfg, "\n"));

    squashed_cfg = config::helpers::mergeNestedMaps(squashed_cfg, cfg);

    logger::trace(
        "++++++++++++ result +++++++++++++++\n{}\n"
        "============== END ================\n",
        fmt::join(squashed_cfg, "\n"));
  }
  return squashed_cfg;
}

/// \brief Remove the protos from merged dictionary
/// \param[in/out] cfg_map - The top level (resolved) config map
void ConfigReader::stripProtos(config::types::CfgMap& cfg_map) {
  // For each entry in the protos map, find the corresponding entry in the resolved config and
  // remove the proto. This isn't strictly necessary, but it simplifies some things later when
  // trying to resolve references and other variables.
  const auto keys = protos_ | ranges::views::keys |
                    ranges::to<std::vector<decltype(protos_)::key_type>> | ranges::actions::sort |
                    ranges::actions::reverse;
  for (auto& key : keys) {
    logger::debug("Removing '{}' from config.", key);
    // Split the keys so we can use them to recurse into the map.
    const auto parts = utils::split(key, '.');

    auto& content = config::helpers::getNestedConfig(cfg_map, parts)->data;

    logger::trace("Final component: \n{}", content.at(parts.back()));
    content.erase(parts.back());
    if (content.empty()) {
      logger::debug("{} is empty and could be removed.", parts | ranges::views::drop_last(1));
    }
  }
}

/// \brief Walk through CfgMap and find all references. Convert them to structs
/// \param[in/out] cfg_map
/// \param[in] base_name - ???
/// \param[in] ref_vars - map of all reference variables available in the current context
void ConfigReader::resolveReferences(config::types::CfgMap& cfg_map, const std::string& base_name,
                                     config::types::RefMap ref_vars) {
  for (auto& kv : cfg_map) {
    const auto& k = kv.first;
    auto& v = kv.second;
    if (v == nullptr) {
      logger::error("base_name: {}", base_name);
      logger::error("Something seems off. Key '{}' is null.", k);
      logger::error("Contents of cfg_map: \n{}", cfg_map);
      continue;
    }
    const auto new_name = utils::makeName(base_name, k);
    if (v->type == config::types::Type::kReference) {
      // When finding a reference, we want to create a new 'struct' given the name, copy any
      // existing reference key/value pairs, and also add any 'proto' key/value pairs. Then, all
      // references need to be resolved. This all needs to be done without destroying the 'proto'
      // as it may be used elsewhere!
      auto v_ref = dynamic_pointer_cast<config::types::ConfigReference>(v);
      auto& p = protos_.at(v_ref->proto);
      logger::trace(
          "Creating struct from reference:\n"
          "reference: \n{}\n"
          "proto: \n{}",
          v, static_pointer_cast<config::types::ConfigBase>(p));
      // Create a new struct from the reference and the proto
      auto new_struct = config::helpers::structFromReference(v_ref, p);
      logger::debug("struct from reference: \n{}",
                    static_pointer_cast<config::types::ConfigBase>(new_struct));
      // If there's a nested dictionary, we want to add any new ref_vars into the existing
      // ref_vars.
      logger::trace("Current ref_vars: {}", ref_vars);
      std::copy(std::begin(v_ref->ref_vars), std::end(v_ref->ref_vars),
                std::inserter(ref_vars, ref_vars.end()));
      logger::trace("Updated ref_vars: {}", ref_vars);

      // Resolve all proto/reference variables based on the values provided.
      config::helpers::replaceProtoVar(new_struct->data, ref_vars);
      // Replace the existing reference with the new struct that was created.
      cfg_map[k] = new_struct;
      // Call recursively in case the current reference has another reference
      resolveReferences(new_struct->data, utils::makeName(new_name, k), ref_vars);

    } else if (v->type == config::types::Type::kStruct) {
      resolveReferences(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data, new_name,
                        ref_vars);
    }
  }
}

void ConfigReader::resolveVarRefs(const config::types::CfgMap& root,
                                  config::types::CfgMap& sub_tree) {
  for (const auto& kv : sub_tree) {
    if (kv.second->type == config::types::Type::kValueLookup) {
      const auto value = config::helpers::getConfigValue(
          root, dynamic_pointer_cast<config::types::ConfigValueLookup>(kv.second));
      logger::debug("Found instance of ValueLookup: {}. Has value={}", kv.second, value);
      sub_tree[kv.first] = value;
    } else if (config::helpers::isStructLike(kv.second)) {
      resolveVarRefs(root, dynamic_pointer_cast<config::types::ConfigStructLike>(kv.second)->data);
    }
  }
}
