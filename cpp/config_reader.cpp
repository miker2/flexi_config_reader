#include <fmt/format.h>

#include <filesystem>
#include <iostream>
#include <range/v3/all.hpp>  // get everything (consider pruning this down a bit)
//#include <range/v3/action/remove_if.hpp>
#include <tao/pegtl.hpp>

#include "config_actions.h"
#include "config_grammar.h"
#include "config_selector.h"
#include "utils.h"

namespace {

auto isStructLike(const std::shared_ptr<config::types::ConfigBase>& el) -> bool {
  return el->type == config::types::Type::kStruct || el->type == config::types::Type::kProto ||
         el->type == config::types::Type::kReference;
}

// Three cases to check for:
//   - Both are dictionaries     - This is okay
//   - Neither are dictionaries  - This is bad - even if the same value, we don't allow duplicates
//   - Only ones is a dictionary - Also bad. We can't handle this one
auto check_for_errors(const config::types::CfgMap& cfg1, const config::types::CfgMap& cfg2,
                      const std::string& key) -> bool {
  const auto dict_count = isStructLike(cfg1.at(key)) + isStructLike(cfg2.at(key));
  if (dict_count == 0) {  // Neither is a dictionary. We don't support duplicate keys
    throw config::DuplicateKeyException(fmt::format("Duplicate key '{}' found at {} and {}!", key,
                                                    cfg1.at(key)->loc(), cfg2.at(key)->loc()));
    // print(f"Found duplicate key: '{key}' with values '{dict1[key]}' and '{dict2[key]}'!!!")
  } else if (dict_count == 1) {  // One dictionary, but not the other, can't merge these
    throw config::MismatchKeyException(
        fmt::format("Mismatch types for key '{}' found at {} and {}! Both keys must point to "
                    "structs, can't merge these.",
                    key, cfg1.at(key)->loc(), cfg2.at(key)->loc()));
  }
  if (cfg1.at(key)->type != cfg2.at(key)->type) {
    throw config::MismatchTypeException(
        fmt::format("Types at key '{}' must match. cfg1 is '{}', cfg2 is '{}'.", key,
                    cfg1.at(key)->type, cfg2.at(key)->type));
  }
  return dict_count == 2;  // All good if both are dictionaries (for now);
}

/* Merge dictionaries recursively and keep all nested keys combined between the two dictionaries.
 * Any key/value pairs that already exist in the leaves of cfg1 will be overwritten by the same
 * key/value pairs from cfg2. */
auto mergeNestedMaps(const config::types::CfgMap& cfg1, const config::types::CfgMap& cfg2)
    -> config::types::CfgMap {
  const auto common_keys =
      ranges::views::set_intersection(cfg1 | ranges::views::keys, cfg2 | ranges::views::keys);
  std::cout << "Common keys: \n";
  for (const auto& k : common_keys) {
    std::cout << k << std::endl;
    check_for_errors(cfg1, cfg2, k);
  }
  std::cout << "~~~~~~~~~~~~~~~~~~~~\n";

  // This over-writes the top level keys in dict1 with dict2. If there are
  // nested dictionaries, we need to handle these appropriately.
  config::types::CfgMap cfg_out{};
  std::copy(std::begin(cfg1), std::end(cfg1), std::inserter(cfg_out, cfg_out.end()));
  std::copy(std::begin(cfg2), std::end(cfg2), std::inserter(cfg_out, cfg_out.end()));
  for (auto& el : cfg_out) {
    const auto& key = el.first;
    if (ranges::find(std::begin(common_keys), std::end(common_keys), key) !=
        std::end(common_keys)) {
      // Find any values in the top-level keys that are also dictionaries.
      // Call this function recursively.
      if (isStructLike(cfg1.at(key)) && isStructLike(cfg2.at(key))) {
        std::cout << fmt::format("Mergeing '{}' (type: {}).", key, cfg_out[key]->type) << std::endl;
        // This means that both the element found in `cfg_out[key]` and those in `cfg1[key]` and
        // `cfg2[key]` are all struct-like elements. We need to convert them all so we can operate
        // on them.
        dynamic_pointer_cast<config::types::ConfigStructLike>(cfg_out[key])->data = mergeNestedMaps(
            dynamic_pointer_cast<config::types::ConfigStructLike>(cfg1.at(key))->data,
            dynamic_pointer_cast<config::types::ConfigStructLike>(cfg2.at(key))->data);
      }
    }
  }

  return cfg_out;
}
}  // namespace

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

    std::cout << "===== Resolving References ========" << std::endl;
    resolveReferences(resolved, "", {});

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
          protos_[new_name] = struct_like;
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
      squashed_cfg = mergeNestedMaps(squashed_cfg, cfg);
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

  void replaceProtoVar(config::types::CfgMap& d, config::types::RefMap& ref_var) {
    for (auto& kv : d) {
      const auto& k = kv.first;
      auto& v = kv.second;
      if (v->type == config::types::Type::kVar) {
        auto v_var = dynamic_pointer_cast<config::types::ConfigVar>(v);
        std::cout << v_var << std::endl;
        // TODO: @@@ Fix this line
        // d[k] = ref_var[v_var->name];
      } else if (v->type == config::types::Type::kValue) {
        auto v_value = dynamic_pointer_cast<config::types::ConfigValue>(v);
        // Find instances of 'ref_var' in 'v' and replace.
        for (auto& rkv : ref_var) {
          const auto& rk = rkv.first;
          auto& rv = rkv.second;
          /*
          // TODO: @@@ Fix these lines
          std::cout << fmt::format("v: {}, rk: ${}, rv: {}", v, rk, rv) << std::endl;
          v = v.replace(f "${rk}", rv);
          std::cout << fmt::format("v: {}, rk: ${{{}}}, rv: {}", v, rk, rv);
          v = v.replace(f "${{{rk}}}", rv);
          std::cout << fmt::format("v: {}", v) << std::endl;
          d[k] = v;
          */
        }
      } else if (isStructLike(v)) {
        replaceProtoVar(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data, ref_var);
      }
    }
  }

  void resolveReferences(config::types::CfgMap& d, const std::string& name,
                         config::types::RefMap ref_vars) {
    /*
    if (ref_vars is None) {
      ref_vars = {};
    }
    */
    for (auto& kv : d) {
      const auto& k = kv.first;
      auto& v = kv.second;
      if (v == nullptr) {
        std::cerr << "name: " << name << std::endl;
        std::cerr << "Something seems off. Key '" << k << "' is null." << std::endl;
        std::cerr << "Contents of d: \n" << d << std::endl;
        continue;
      }
      const auto new_name = config::utils::makeName(name, k);
      if (v->type == config::types::Type::kReference) {
        auto v_ref = dynamic_pointer_cast<config::types::ConfigReference>(v);
        auto& p = protos_.at(v_ref->proto);
        std::cout << "p: " << p << std::endl;
        // Make a copy of the data contained in the proto. Careful here, as we might need a deep
        // copy of this data.
        auto r = p->data;
        std::cout << fmt::format("r: {}", v) << std::endl;
        ref_vars = v_ref->ref_vars;
        for (auto& el : v_ref->data) {
          if (isStructLike(el.second)) {
            // append to proto
            std::cout << fmt::format("new keys: {}", el.second) << std::endl;
            r = mergeNestedMaps(
                r, dynamic_pointer_cast<config::types::ConfigStructLike>(el.second)->data);
          }
        }
        std::cout << "Ref vars: \n" << ref_vars << std::endl;
        replaceProtoVar(r, ref_vars);
        /* // TODO: Figure out what is supposed to happen on the next line
        d[k] = r;
        */
        // Call recursively in case the current reference has another reference
        resolveReferences(r, utils::makeName(new_name, k), ref_vars);

      } else if (isStructLike(v)) {
        resolveReferences(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data, new_name,
                          ref_vars);
      }
    }
  }

  void resolveVarRefs() {}

  config::ActionData out_;
  std::map<std::string, std::shared_ptr<config::types::ConfigStructLike>> protos_{};
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
