#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/view/drop_last.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/set_algorithm.hpp>
#include <regex>

#include "config_classes.h"
#include "config_exceptions.h"

#define CONFIG_HELPERS_DEBUG 0

namespace config::helpers {

inline auto isStructLike(const std::shared_ptr<config::types::ConfigBase>& el) -> bool {
  return el->type == config::types::Type::kStruct || el->type == config::types::Type::kProto ||
         el->type == config::types::Type::kReference;
}

// Three cases to check for:
//   - Both are dictionaries     - This is okay
//   - Neither are dictionaries  - This is bad - even if the same value, we don't allow duplicates
//   - Only ones is a dictionary - Also bad. We can't handle this one
inline auto checkForErrors(const config::types::CfgMap& cfg1, const config::types::CfgMap& cfg2,
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
inline auto mergeNestedMaps(const config::types::CfgMap& cfg1, const config::types::CfgMap& cfg2)
    -> config::types::CfgMap {
  const auto common_keys =
      ranges::views::set_intersection(cfg1 | ranges::views::keys, cfg2 | ranges::views::keys);

  for (const auto& k : common_keys) {
    checkForErrors(cfg1, cfg2, k);
  }

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

inline auto structFromReference(std::shared_ptr<config::types::ConfigReference>& ref,
                                const std::shared_ptr<config::types::ConfigProto>& proto)
    -> std::shared_ptr<config::types::ConfigStruct> {
  // NOTE: This will not fill in any of the reference variables. It will only generate a struct from
  // an existing reference and proto object.

  // First, create the new struct based on the reference data.
  auto struct_out = std::make_shared<config::types::ConfigStruct>(ref->name, ref->depth);
#if CONFIG_HELPERS_DEBUG
  std::cout << "New struct: \n";
  std::cout << struct_out << "\n";
#endif
  // Next, move the data from the reference to the struct:
  struct_out->data.merge(ref->data);
#if CONFIG_HELPERS_DEBUG
  std::cout << "Data added: \n";
  std::cout << struct_out << "\n";
#endif
  // Next, we need to copy the values from the proto into the reference (need a deep-copy here so
  // that modifying the `std::shared_ptr` objects copied into the reference don't affect those in
  // the proto.
  for (const auto& el : proto->data) {
    if (struct_out->data.contains(el.first) && proto->data.contains(el.first)) {
      config::helpers::checkForErrors(struct_out->data, proto->data, el.first);
    }
    // std::shared_ptr<config::types::ConfigBase> value = el.second->clone();
    struct_out->data[el.first] = std::move(el.second->clone());
  }
  return struct_out;
}

/// \brief Finds all uses of 'ConfigVar' in the contents of a proto and replaces them
/// \param[in/out] cfg_map - Contents of a proto
/// \param[in] ref_vars - All of the available 'ConfigVar's in the reference
inline void replaceProtoVar(config::types::CfgMap& cfg_map, const config::types::RefMap& ref_vars) {
#if CONFIG_HELPERS_DEBUG
  std::cout << "replaceProtoVars --- \n";
  std::cout << cfg_map << std::endl;
  std::cout << "  -- ref_vars: \n";
  std::cout << ref_vars << std::endl;
  std::cout << "  -- END --\n";
#endif
  for (auto& kv : cfg_map) {
    const auto& k = kv.first;
    auto& v = kv.second;
    if (v->type == config::types::Type::kVar) {
      auto v_var = dynamic_pointer_cast<config::types::ConfigVar>(v);
      // Pull the value from the reference vars and add it to the structure.
      if (!ref_vars.contains(v_var->name)) {
        throw config::UndefinedReferenceVarException(
            fmt::format("Attempting to replace '{}' with undefined var: '{}'.", k, v_var->name));
      }
      cfg_map[k] = ref_vars.at(v_var->name);
    } else if (v->type == config::types::Type::kString) {
      auto v_value = dynamic_pointer_cast<config::types::ConfigValue>(v);

      // Before doing any of this, maybe check if there is a "$" in v_value->value, otherwise, no
      // sense in doing this loop.
      const auto var_pos = v_value->value.find('$');
      if (var_pos == std::string::npos) {
        // No variables. Let's skip.
        std::cout << "No variables in " << v_value << ". Skipping..." << std::endl;
        continue;
      }

      auto out = v_value->value;
      // Find instances of 'ref_vars' in 'v' and replace.
      for (auto& rkv : ref_vars) {
        const auto& rk = rkv.first;
        auto rv = dynamic_pointer_cast<config::types::ConfigValue>(rkv.second);

        std::cout << "v: " << out << ", rk: " << rk << ", rv: " << rv << std::endl;
        // Strip off any leading or trailing quotes from the replacement value. If the replacement
        // value is not a string, this is a no-op.
        const auto replacement = utils::trim(rv->value, "\\\"");
        out = std::regex_replace(out, std::regex("\\" + rk), replacement);
        // Turn the $VAR version into ${VAR} in case that is used within a string as well. Throw
        // in escape characters as this will be used in a regular expression.
        const auto bracket_var = std::regex_replace(rk, std::regex("\\$(.+)"), "\\$\\{$1\\}");
        std::cout << "v: " << out << ", rk: " << bracket_var << ", rv: " << rv << std::endl;
        out = std::regex_replace(out, std::regex(bracket_var), replacement);
        std::cout << "out: " << out << std::endl;
      }
      // Replace the existing value with the new value.
      auto new_value = std::make_shared<config::types::ConfigValue>(out, v->type);
      new_value->line = v->line;
      new_value->source = v->source;
      cfg_map[k] = std::move(new_value);
    } else if (config::helpers::isStructLike(v)) {
      std::cout << "At '" << k << "', found " << v->type << std::endl;
      // Recurse deeper into the structure in order to replace more variables.
      replaceProtoVar(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data, ref_vars);
    }
  }
  std::cout << "~~~~~ Result ~~~~~\n";
  std::cout << cfg_map << std::endl;
  std::cout << "===== RPV DONE =====\n";
}

inline auto getNestedConfig(const config::types::CfgMap& cfg, const std::vector<std::string>& keys)
    -> std::shared_ptr<config::types::ConfigStructLike> {
  // Keep track of the re-joined keys (from the front to the back) as we recurse into the map in
  // order to make error messages more useful.
  std::string rejoined = "";
  // Start with the base config tree and work our way down through the keys.
  auto* content = &cfg;
  // Walk through each part of the flat key (except the last one, as it will be the one that
  // contains the data.
  std::shared_ptr<config::types::ConfigStructLike> struct_like;
  for (const auto& key : keys | ranges::views::drop_last(1)) {
    rejoined = utils::makeName(rejoined, key);
    // This may be uneccessary error checking (if this is a part of a flat key, then it must be
    // a nested structure), but we check here that this object is a `StructLike` object,
    // otherwise we can't access the `data` member.
    struct_like = dynamic_pointer_cast<config::types::ConfigStructLike>(content->at(key));
    if (struct_like == nullptr) {
      throw config::InvalidTypeException(
          fmt::format("Expected value at '{}' to be a struct-like object, but got {} type instead.",
                      rejoined, content->at(key)->type));
    }
    // Pull out the contents of the struct-like and move on to the next iteration.
    content = &(struct_like->data);
  }

  return struct_like;
}

inline auto getNestedConfig(const config::types::CfgMap& cfg, const std::string& flat_key)
    -> std::shared_ptr<config::types::ConfigStructLike> {
  const auto keys = utils::split(flat_key, '.');
  return getNestedConfig(cfg, keys);
}

inline auto getConfigValue(const config::types::CfgMap& cfg,
                           const std::shared_ptr<config::types::ConfigValueLookup>& var)
    -> std::shared_ptr<config::types::ConfigBase> {
  // Get the struct-like object containing the last key of the ConfigValueLookup:
  const auto struct_like = getNestedConfig(cfg, var->keys);
  // Extract the value from the struct-like object by accessing the internal data and locating the
  // requested key.
  if (struct_like != nullptr) {
    return struct_like->data.at(var->keys.back());
  }

  return cfg.at(var->keys.back());
}

inline void removeEmpty(config::types::CfgMap& cfg) {
  std::vector<std::remove_reference_t<decltype(cfg)>::key_type> to_erase{};
  for (auto& kv : cfg) {
    if (kv.second->type == config::types::Type::kStruct) {
      auto s = dynamic_pointer_cast<config::types::ConfigStruct>(kv.second);
      removeEmpty(s->data);
      if (s->data.empty()) {
        std::cout << " !!! Removing {} !!!\n";
        to_erase.push_back(kv.first);
      }
    }
  }
  // Erase the key/value pairs in a separate loop (doing this in the main loop was causing a
  // segfault.
  for (const auto& key : to_erase) {
    cfg.erase(key);
  }
}

}  // namespace config::helpers
