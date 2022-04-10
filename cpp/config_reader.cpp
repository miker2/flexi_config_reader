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

    stripProtos();

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

  void stripProtos() {
    // Remove the protos from merged dictionary
    /*
    for p in self._protos.keys():
      parts = p.split('.')
      tmp = d
      for i in range(len(parts)-1):
        tmp = tmp[parts[i]]
      del tmp[parts[-1]]
    */
  }

  /*
  /// \brief Finds all uses of 'ConfigVar' in the contents of a proto and replaces them
  /// \param[in/out] cfg_map - Contents of a proto
  /// \param[in] ref_vars - All of the available 'ConfigVar's in the reference
  void replaceProtoVar(config::types::CfgMap& cfg_map, const config::types::RefMap& ref_vars) {
    std::cout << "replaceProtoVars --- \n";
    std::cout << cfg_map << std::endl;
    std::cout << "  -- ref_vars: \n";
    std::cout << ref_vars << std::endl;

    for (auto& kv : cfg_map) {
      const auto& k = kv.first;
      auto& v = kv.second;
      if (v->type == config::types::Type::kVar) {
        auto v_var = dynamic_pointer_cast<config::types::ConfigVar>(v);
        std::cout << v_var << std::endl;
        // Pull the value from the reference vars and add it to the structure.
        cfg_map[k] = ref_vars.at(v_var->name);
      } else if (v->type == config::types::Type::kString) {
        // TODO: We don't currently distinguish between the various value types here, but we
        // probably should because we really only want to do this if there is an actual string
        // (rather than some other value type).
        auto v_value = dynamic_pointer_cast<config::types::ConfigValue>(v);

        // Before doing any of this, maybe check if there is a "$" in v_value->value, otherwise, no
        // sense in doing this loop.
        const auto var_pos = v_value->value.find('$');
        if (var_pos == std::string::npos) {
          // No variables. Let's skip.
          std::cout << "No variables in " << v_value << ". Skipping..." << std::endl;
          continue;
        }

        // Find instances of 'ref_vars' in 'v' and replace.
        for (auto& rkv : ref_vars) {
          const auto& rk = rkv.first;
          auto rv = dynamic_pointer_cast<config::types::ConfigValue>(rkv.second);

          std::cout << "v: " << v << ", rk: " << rk << ", rv: " << rv << std::endl;
          auto out = std::regex_replace(v_value->value, std::regex("\\" + rk), rv->value);
          // Turn the $VAR version into ${VAR} in case that is used within a string as well. Throw
          // in escape characters as this will be used in a regular expression.
          const auto bracket_var = std::regex_replace(rk, std::regex("\\$(.+)"), "\\$\\{$1\\}");
          std::cout << "v: " << v << ", rk: " << bracket_var << ", rv: " << rv << std::endl;
          out = std::regex_replace(out, std::regex(bracket_var), rv->value);
          std::cout << "out: " << out << std::endl;

          // Replace the existing value with the new value.
          cfg_map[k] = std::make_shared<config::types::ConfigValue>(out);
        }
      } else if (config::helpers::isStructLike(v)) {
        replaceProtoVar(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data, ref_vars);
      }
    }
    std::cout << cfg_map << std::endl;
    std::cout << "===== RPV DONE =====\n";
  }
  */

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
        std::cout << "p: " << p << std::endl;
        auto new_struct = config::helpers::structFromReference(v_ref, p);
        std::cout << "struct from reference: \n" << new_struct << std::endl;
        // Make a copy of the data contained in the proto. Careful here, as we might need a deep
        // copy of this data.
        auto r = p->data;
        std::cout << fmt::format("r: {}", v) << std::endl;
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
