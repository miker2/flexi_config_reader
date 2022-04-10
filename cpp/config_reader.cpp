#include <fmt/format.h>

#include <filesystem>
#include <iostream>
#include <range/v3/all.hpp>  // get everything (consider pruning this down a bit)
#include <regex>
#include <tao/pegtl.hpp>

#include "config_actions.h"
#include "config_classes.h"
#include "config_exceptions.h"
#include "config_grammar.h"
#include "config_helpers.h"
#include "utils.h"

class ConfigReader {
 public:
  ConfigReader() {}

  auto parse(const std::filesystem::path& cfg_filename) -> bool {
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

      std::cout << "\n";
      std::cout << "  Parse " << (success ? "success" : "failure") << std::endl;
      std::cout << "  cfg_res size: " << out_.cfg_res.size() << std::endl;

      std::cout << "\noutput: \n";
      out_.print();
    } catch (const peg::parse_error& e) {
      success = false;
      std::cout << "!!!\n";
      std::cout << "  Parser failure!\n";
      const auto p = e.positions().front();
      std::cout << e.what() << '\n'
                << cfg_file.line_at(p) << '\n'
                << std::setw(p.column) << '^' << '\n';
      std::cout << "!!!\n";
    }

    std::cout << "Flattened: \n";
    config::types::CfgMap flat{};
    for (const auto& e : out_.cfg_res) {
      flat = flattenAndFindProtos(e, "", flat);
    }
    std::cout << flat << std::endl;

    std::cout << "Protos: \n";
    protos_ | ranges::views::keys;  // | ranges::views::for_each([](const std::string& s) {
                                    // std::cout << s << std::endl; });
    for (const auto& s : (protos_ | ranges::views::keys)) {
      std::cout << s << std::endl;
    }

    auto resolved = mergeNested(out_.cfg_res);

    std::cout << std::string(35, '=') << " Strip Protos " << std::string(35, '=') << std::endl;
    stripProtos(resolved);
    std::cout << " --- Result of 'stripProtos':\n";
    std::cout << protos_ << std::endl;
    std::cout << resolved << std::endl;

    std::cout << std::string(35, '=') << " Resolving References " << std::string(35, '=')
              << std::endl;
    resolveReferences(resolved, "", {});
    std::cout << resolved;

    std::cout << "===== Resolve variable references ====" << std::endl;
    resolveVarRefs();

    return success;
  }

 private:
  auto flattenAndFindProtos(const config::types::CfgMap& in, const std::string& base_name,
                            config::types::CfgMap flattened = {}) -> config::types::CfgMap {
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
        // std::cout << new_name << " = " << e.second << std::endl;
      }
    }
    return flattened;
  }

  auto mergeNested(const std::vector<config::types::CfgMap>& in) -> config::types::CfgMap {
    if (in.empty()) {
      // TODO: Throw exception here? How to handle empty vector?
      return {};
    }

    // This whole function is quite inefficient, but it works, which is a start:

    // Start with the first element:
    config::types::CfgMap squashed_cfg = in[0];

    // Accumulate all of the other elements of the vector into the CfgMap.
    for (auto& cfg : ranges::views::tail(in)) {
      std::cout << "======= Working on merging: =======\n";
      std::cout << squashed_cfg << std::endl;
      std::cout << "++++++++++++++ and ++++++++++++++++\n";
      std::cout << cfg << std::endl;
      std::cout << "++++++++++++ result +++++++++++++++\n";
      squashed_cfg = config::helpers::mergeNestedMaps(squashed_cfg, cfg);
      std::cout << squashed_cfg << std::endl;
      std::cout << "============== END ================\n";
    }
    return squashed_cfg;
  }

  /// \brief Remove the protos from merged dictionary
  /// \param[in/out] cfg_map - The top level (resolved) config map
  void stripProtos(config::types::CfgMap& cfg_map) {
    // For each entry in the protos map, find the corresponding entry in the resolved config and
    // remove the proto. This isn't strictly necessary, but it simplifies some things later when
    // trying to resolve references and other variables.
    for (auto& kv : protos_) {
      std::cout << "Removing '" << kv.first << "' from config." << std::endl;
      // Split the keys so we can use them to recurse into the map.
      const auto parts = utils::split(kv.first, '.');
      // Keep track of the re-joined keys (from the front to the back) as we recurse into the map in
      // order to make error messages more useful.
      std::string rejoined = "";
      // This isn't very efficient as there may be common trunks between the different protos, but
      // restart with the root node each time.
      config::types::CfgMap* content = &cfg_map;
      // Walk through each part of the flat key (except the last one, as it will be the one to be
      // removed).
      for (size_t i = 0; i < parts.size() - 1; ++i) {
        rejoined = utils::makeName(rejoined, parts[i]);
        // This may be uneccessary error checking (if this is a part of a flat key, then it must be
        // a nested structure), but we check here that this object is a `StructLike` object,
        // otherwise we can't access the data member.
        const auto v = dynamic_pointer_cast<config::types::ConfigStructLike>(content->at(parts[i]));
        if (v == nullptr) {
          throw config::InvalidTypeException(fmt::format(
              "Expected value at '{}' to be a struct-like object, but got {} type instead.",
              rejoined, content->at(parts[i])->type));
        }
        // Pull out the contents of the struct-like and move on to the next iteration.
        content = &(v->data);
        std::cout << "At '" << parts[i] << "':\n";
        std::cout << *content << std::endl;
      }
      std::cout << "Final component: " << parts.back() << " = " << content->at(parts.back())
                << std::endl;
      content->erase(parts.back());
    }
  }

  /// \brief Walk through CfgMap and find all references. Convert them to structs
  /// \param[in/out] cfg_map
  /// \param[in] base_name - ???
  /// \param[in] ref_vars - map of all reference variables available in the current context
  void resolveReferences(config::types::CfgMap& cfg_map, const std::string& base_name,
                         config::types::RefMap ref_vars) {
    for (auto& kv : cfg_map) {
      const auto& k = kv.first;
      auto& v = kv.second;
      if (v == nullptr) {
        std::cerr << "base_name: " << base_name << std::endl;
        std::cerr << "Something seems off. Key '" << k << "' is null." << std::endl;
        std::cerr << "Contents of cfg_map: \n" << cfg_map << std::endl;
        continue;
      }
      const auto new_name = utils::makeName(base_name, k);
      if (v->type == config::types::Type::kReference) {
        // TODO: When finding a reference, we want to create a new 'struct' given the name, copy any
        // existing reference key/value pairs, and also add any 'proto' key/value pairs. Then, all
        // references need to be resolved. This all needs to be done without destroying the 'proto'
        // as it may be used elsewhere!
        auto v_ref = dynamic_pointer_cast<config::types::ConfigReference>(v);
        auto& p = protos_.at(v_ref->proto);
        std::cout << fmt::format("r: {}", v) << std::endl;
        std::cout << "p: " << p << std::endl;
        auto new_struct = config::helpers::structFromReference(v_ref, p);
        std::cout << "struct from reference: \n" << new_struct << std::endl;
        // Make a copy of the data contained in the proto. Careful here, as we might need a deep
        // copy of this data.
        auto r = p->data;
        // If there's a nested dictionary, we want to add any new ref_vars into the existing
        // ref_vars.
        std::cout << "Current ref_vars: \n" << ref_vars << std::endl;
        std::copy(std::begin(v_ref->ref_vars), std::end(v_ref->ref_vars),
                  std::inserter(ref_vars, ref_vars.end()));
        std::cout << "Updated ref_vars: \n" << ref_vars << std::endl;
        for (auto& el : v_ref->data) {
          if (config::helpers::isStructLike(el.second)) {
            // append to proto
            std::cout << fmt::format("new keys: {}", el.second) << std::endl;
            r = config::helpers::mergeNestedMaps(
                r, dynamic_pointer_cast<config::types::ConfigStructLike>(el.second)->data);
          }
        }
        std::cout << "Ref vars: \n" << ref_vars << std::endl;
        config::helpers::replaceProtoVar(new_struct->data, ref_vars);
        // Replace the existing reference with the new struct that was created.
        cfg_map[k] = new_struct;
        // Call recursively in case the current reference has another reference
        resolveReferences(r, utils::makeName(new_name, k), ref_vars);

      } else if (config::helpers::isStructLike(v)) {
        resolveReferences(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data, new_name,
                          ref_vars);
      }
    }
  }

  void resolveVarRefs() {}

  config::ActionData out_;
  std::map<std::string, std::shared_ptr<config::types::ConfigProto>> protos_{};
};

auto main(int argc, char* argv[]) -> int {
  if (argc != 2) {
    std::cerr << "No file specified.\n";
    std::cerr << "usage: " << std::filesystem::path(argv[0]).filename().string() << " CFG_FILE"
              << std::endl;
    return -1;
  }

  ConfigReader cfg;
  const auto success = cfg.parse(argv[1]);

  return (success ? 0 : 1);
}
