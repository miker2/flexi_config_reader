#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <range/v3/action/remove_if.hpp>
#include <range/v3/action/reverse.hpp>
#include <range/v3/action/sort.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/drop_last.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/tail.hpp>
#include <sstream>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/trace.hpp>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/exceptions.h"
#include "flexi_cfg/config/grammar.h"
#include "flexi_cfg/config/helpers.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/parser.h"
#include "flexi_cfg/reader.h"
#include "flexi_cfg/utils.h"

namespace {
constexpr bool STRIP_PROTOS{true};

template <typename INPUT>
auto parseCommon(INPUT& input, flexi_cfg::config::ActionData& output) -> bool {
  bool success = true;
  try {
    success = peg::parse<peg::must<flexi_cfg::config::grammar>, flexi_cfg::config::action,
                         flexi_cfg::config::control>(input, output);
    // If parsing is successful, all of these containers should be empty (consumed into
    // 'output.cfg_res').
    success &= output.keys.empty();
    success &= output.flat_keys.empty();
    success &= output.objects.empty();
    success &= output.obj_res == nullptr;

    // Eliminate any vector elements with an empty map.
    output.cfg_res |= ranges::actions::remove_if(
        [](const flexi_cfg::config::types::CfgMap& m) { return m.empty(); });

    if (!success) {
      const auto position = input.position();
      flexi_cfg::logger::critical("  Parse failure");
      flexi_cfg::logger::error("  cfg_res size: {}", output.cfg_res.size());

      std::stringstream ss;
      output.print(ss);
      flexi_cfg::logger::error("Incomplete output: \n{}", ss.str());
      flexi_cfg::logger::error("Error at: {} : {}", position.source, position.line);

      // Print a trace if a failure occured.
      // input.restart();
      // peg::standard_trace<flexi_cfg::config::grammar>(input);
      return success;
    }
  } catch (const peg::parse_error& e) {
    success = false;
    flexi_cfg::logger::critical("!!!");
    flexi_cfg::logger::critical("  Parser failure!");
    const auto p = e.positions().front();
    flexi_cfg::logger::critical("{}", e.what());
    flexi_cfg::logger::critical("{}", input.line_at(p));
    flexi_cfg::logger::critical("{}^", std::string(p.column - 1, ' '));
    std::stringstream ss;
    output.print(ss);
    flexi_cfg::logger::critical("Partial output: \n{}", ss.str());
    flexi_cfg::logger::critical("!!!");
    return success;
  }

  return success;
}

auto mergeNested(const std::vector<flexi_cfg::config::types::CfgMap>& in)
    -> flexi_cfg::config::types::CfgMap {
  if (in.empty()) {
    // TODO(miker2): Throw exception here? How to handle empty vector?
    return {};
  }

  // This whole function is quite inefficient, but it works, which is a start:

  // Start with the first element:
  flexi_cfg::config::types::CfgMap squashed_cfg = in[0];

  // Accumulate all of the other elements of the vector into the CfgMap.
  for (const auto& cfg : ranges::views::tail(in)) {
    flexi_cfg::logger::trace(
        "======= Working on merging: =======\n{}\n"
        "++++++++++++++ and ++++++++++++++++\n{}",
        fmt::join(squashed_cfg, "\n"), fmt::join(cfg, "\n"));

    squashed_cfg = flexi_cfg::config::helpers::mergeNestedMaps(squashed_cfg, cfg);

    flexi_cfg::logger::trace(
        "++++++++++++ result +++++++++++++++\n{}\n"
        "============== END ================\n",
        fmt::join(squashed_cfg, "\n"));
  }
  return squashed_cfg;
}

}  // namespace

namespace flexi_cfg {

auto Parser::parse(const std::filesystem::path& cfg_filename,
                   std::optional<std::filesystem::path> root_dir) -> Reader {
  config::ActionData state;

  std::filesystem::path input_file;
  if (root_dir.has_value()) {
    input_file = root_dir.value() / cfg_filename;
    state.base_dir = root_dir.value();
  } else {
    input_file = cfg_filename;
    state.base_dir = cfg_filename.parent_path();
  }
  peg::file_input cfg_file(input_file);

  // TODO(miker2): Do something smarter if "parseCommon" fails!
  parseCommon(cfg_file, state);

  Parser parser;
  return Reader(parser.resolveConfig(state));
}

auto Parser::parseFromString(std::string_view cfg_string, std::string_view source) -> Reader {
  peg::memory_input cfg_file(cfg_string, source);
  config::ActionData state;

  // TODO(miker2): Do something smarter if "parseCommon" fails!
  parseCommon(cfg_file, state);

  Parser parser;
  return Reader(parser.resolveConfig(state));
}

auto Parser::resolveConfig(config::ActionData& state) -> const config::types::CfgMap& {
  config::types::CfgMap flat{};
  for (const auto& e : state.cfg_res) {
    flat = flattenAndFindProtos(e, "", flat);
  }
  logger::debug("Flattened: \n {}", fmt::join(flat, "\n "));

  logger::debug("Protos: \n  {}", fmt::join(protos_ | ranges::views::keys, "\n  "));

  logger::debug("Overrides: \n {}", fmt::join(state.override_values, "\n "));

  static const std::string debug_sep(35, '=');
  logger::debug("{0} Resolving References {0}", debug_sep);
  // Iterate over each map in the parse results and resolve any references. This is done here
  // because we don't support combining references & structs, so we need all references to be
  // resolved to their equivalent struct.
  for (auto& e : state.cfg_res) {
    resolveReferences(e, "", {});
    logger::trace("Resolved results: \n{}", fmt::join(e, "\n"));
  }
  logger::debug("{0} Done resolving refs {0}", debug_sep);

  cfg_data_ = mergeNested(state.cfg_res);

  validateAndApplyOverrides(state, cfg_data_);

  if (STRIP_PROTOS) {
    // Stripping protos is not strictly necessary, but it cleans up the resulting config file.
    logger::trace("{0} Strip Protos {0}", debug_sep);
    stripProtos(cfg_data_);
    logger::trace(" --- Result of 'stripProtos':\n{}", fmt::join(cfg_data_, "\n"));
  }

  // Unflatten any flat keys:
  const auto flat_keys =
      cfg_data_ | ranges::views::keys |
      ranges::views::filter([](auto& key) { return key.find(".") != std::string::npos; }) |
      ranges::to<std::vector<std::string>> | ranges::actions::sort | ranges::actions::reverse;
  logger::debug("The following keys need to be flattened: {}", flat_keys);
  for (const auto& key : flat_keys) {
    config::helpers::unflatten(key, cfg_data_);
  }

  config::helpers::resolveVarRefs(cfg_data_, cfg_data_);

  config::helpers::evaluateExpressions(cfg_data_);

  // Removes empty structs, fixes incorrect depth, etc.
  config::helpers::cleanupConfig(cfg_data_);

  // fmt::print("config:\n{}", cfg_data_);

  return cfg_data_;
}

auto Parser::flattenAndFindProtos(const config::types::CfgMap& in, const std::string& base_name,
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

void Parser::resolveReferences(config::types::CfgMap& cfg_map, const std::string& base_name,
                               const config::types::RefMap& ref_vars,
                               const std::vector<std::string>& refd_protos) const {
  for (auto& kv : cfg_map) {
    const auto& k = kv.first;
    auto& v = kv.second;
    if (v == nullptr) {
      logger::error("base_name: {}", base_name);
      logger::error("Something seems off. Key '{}' is null.", k);
      logger::error("Contents of cfg_map: \n{}", cfg_map);
      THROW_EXCEPTION(config::InvalidConfigException,
                      "\n"
                      "\tbase_name: {}\n"
                      "\tSomething is wrong. Key '{}' is NULL.\n"
                      "\tContents of cfg_map: \n"
                      "\t{}",
                      base_name, k, cfg_map);
    }
    const auto new_name = utils::makeName(base_name, k);
    if (v->type == config::types::Type::kProto) {
      // If a proto has been found, don't go any further. We don't want to resolve nested references
      // within a proto until that proto has been referenced.
      logger::trace("Found nested proto '{}'. Skipping...", new_name);
      continue;
    }
    if (v->type == config::types::Type::kReference) {
      // When finding a reference, we want to create a new 'struct' given the name, copy any
      // existing reference key/value pairs, and also add any 'proto' key/value pairs. Then, all
      // references need to be resolved. This all needs to be done without destroying the 'proto'
      // as it may be used elsewhere!
      auto v_ref = dynamic_pointer_cast<config::types::ConfigReference>(v);
      if (!protos_.contains(v_ref->proto)) {
        THROW_EXCEPTION(config::UndefinedProtoException,
                        "Unable to find proto '{}' referenced by '{}'.", v_ref->proto, new_name);
      }
      if (utils::contains(refd_protos, v_ref->proto)) {
        THROW_EXCEPTION(config::CyclicReferenceException,
                        "Cyclic reference found when resolving reference at '{}'. Proto '{}' "
                        "already referenced.\n  References: [{}]",
                        new_name, v_ref->proto, fmt::join(refd_protos, " -> "));
      }
      // Need to make a copy of the already referenced protos and add the new proto to it.
      auto updated_refd_protos = refd_protos;
      updated_refd_protos.emplace_back(v_ref->proto);  // This will be passed to recursive calls.
      const auto& p = protos_.at(v_ref->proto);
      logger::trace(
          "Creating struct from reference:\n"
          "reference: \n{}\n"
          "proto: \n{}",
          v, p);
      // Create a new struct from the reference and the proto
      auto new_struct = config::helpers::structFromReference(v_ref, p);
      logger::debug("struct from reference: \n{}", new_struct);
      // If there's a nested dictionary, we want to add any new ref_vars into the existing
      // ref_vars.
      logger::trace("Current ref_vars: {}", ref_vars);
      logger::trace("New ref_vars: {}", v_ref->ref_vars);
      auto updated_ref_vars = ref_vars;
      std::copy(std::begin(v_ref->ref_vars), std::end(v_ref->ref_vars),
                std::inserter(updated_ref_vars, updated_ref_vars.end()));
      logger::trace("Updated ref_vars: {}", updated_ref_vars);

      // Resolve all proto/reference variables based on the values provided.
      config::helpers::replaceProtoVar(new_struct->data, updated_ref_vars);
      // Replace the existing reference with the new struct that was created.
      cfg_map[k] = new_struct;
      // Call recursively in case the current reference has another reference
      resolveReferences(new_struct->data, new_name, updated_ref_vars, updated_refd_protos);

    } else if (v->type == config::types::Type::kStructInProto) {
      // Resolve all proto/reference variables based on the values provided.
      auto struct_like = dynamic_pointer_cast<config::types::ConfigStructLike>(v);
      config::helpers::replaceProtoVar(struct_like->data, ref_vars);

      resolveReferences(struct_like->data, new_name, ref_vars, refd_protos);
    } else if (v->type == config::types::Type::kStruct) {
      resolveReferences(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data, new_name,
                        ref_vars, refd_protos);
    }
  }
}

void Parser::validateAndApplyOverrides(const config::ActionData& state,
                                       config::types::CfgMap& cfg_map) const {
  /*
  for (const auto& override : state.override_values) {
    const auto parts = utils::split(override.first, '.');
    if (parts.size() == 1) {
      // If the override key is at the top level, just apply it.
      cfg_map[parts[0]] = override.second;
    } else {
      // Otherwise, we need to find the nested key and apply the override.
      auto& content = config::helpers::getNestedConfig(cfg_map, parts)->data;
      content[parts.back()] = override.second;
    }
  }
  */
}

void Parser::stripProtos(config::types::CfgMap& cfg_map) const {
  // For each entry in the protos map, find the corresponding entry in the resolved config and
  // remove the proto. This isn't strictly necessary, but it simplifies some things later when
  // trying to resolve references and other variables.
  const auto keys = protos_ | ranges::views::keys |
                    ranges::to<std::vector<decltype(protos_)::key_type>> | ranges::actions::sort |
                    ranges::actions::reverse;
  for (const auto& key : keys) {
    logger::debug("Removing '{}' from config.", key);
    // Split the keys so we can use them to recurse into the map.
    const auto parts = utils::split(key, '.');

    auto& content =
        parts.size() == 1 ? cfg_map : config::helpers::getNestedConfig(cfg_map, parts)->data;

    logger::trace("Final component: \n{}", content.at(parts.back()));
    content.erase(parts.back());
    if (content.empty()) {
      logger::debug("{} is empty and could be removed.", parts | ranges::views::drop_last(1));
    }
  }
}

}  // namespace flexi_cfg
