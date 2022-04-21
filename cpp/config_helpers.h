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
#include <span>

#include "config_classes.h"
#include "config_exceptions.h"
#include "logger.h"
#include "utils.h"

#define CONFIG_HELPERS_DEBUG 0

namespace config {
// Create a set of traits for acceptable containers for holding elements of type `kList`. We define
// an implementation for the possible types here, but the actual trait is defined below.
namespace accepts_list_impl {
template <typename T>
struct accepts_list : std::false_type {};
template <typename T, std::size_t N>
struct accepts_list<std::array<T, N>> : std::true_type {};
template <typename... Args>
struct accepts_list<std::vector<Args...>> : std::true_type {};
}  // namespace accepts_list_impl

// This is the actual trait to be used. This allows us to decay the type so it will work for
// references, pointers, etc.
template <typename T>
struct accepts_list {
  static constexpr bool const value = accepts_list_impl::accepts_list<std::decay_t<T>>::value;
};

template <typename T>
inline constexpr bool accepts_list_v = accepts_list<T>::value;
}  // namespace config

namespace config::helpers {

inline auto isStructLike(const std::shared_ptr<config::types::ConfigBase>& el) -> bool {
  // TODO: Change this to be a `dynamic_pointer_cast`. As is, it's a bit of a maintenance burden.
  return el->type == config::types::Type::kStruct ||
         el->type == config::types::Type::kStructInProto ||
         el->type == config::types::Type::kProto || el->type == config::types::Type::kReference;
}

// Three cases to check for:
//   - Both are dictionaries     - This is okay
//   - Neither are dictionaries  - This is bad - even if the same value, we don't allow duplicates
//   - Only ones is a dictionary - Also bad. We can't handle this one
inline auto checkForErrors(const config::types::CfgMap& cfg1, const config::types::CfgMap& cfg2,
                           const std::string& key) -> bool {
  const auto dict_count = isStructLike(cfg1.at(key)) + isStructLike(cfg2.at(key));
  if (dict_count == 0) {  // Neither is a dictionary. We don't support duplicate keys
    THROW_EXCEPTION(config::DuplicateKeyException, "Duplicate key '{}' found at {} and {}!", key,
                    cfg1.at(key)->loc(), cfg2.at(key)->loc());
    // print(f"Found duplicate key: '{key}' with values '{dict1[key]}' and '{dict2[key]}'!!!")
  } else if (dict_count == 1) {  // One dictionary, but not the other, can't merge these
    THROW_EXCEPTION(config::MismatchKeyException,
                    "Mismatch types for key '{}' found at {} and {}! Both keys must point to "
                    "structs, can't merge these.",
                    key, cfg1.at(key)->loc(), cfg2.at(key)->loc());
  }
  if (cfg1.at(key)->type != cfg2.at(key)->type) {
    THROW_EXCEPTION(config::MismatchTypeException,
                    "Types at key '{}' must match. cfg1 is '{}', cfg2 is '{}'.", key,
                    cfg1.at(key)->type, cfg2.at(key)->type);
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
  logger::debug("New struct: \n{}", static_pointer_cast<config::types::ConfigBase>(struct_out));
#endif
  // Next, move the data from the reference to the struct:
  struct_out->data.merge(ref->data);
#if CONFIG_HELPERS_DEBUG
  logger::debug("Data added: \n{}", static_pointer_cast<config::types::ConfigBase>(struct_out));
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
  logger::trace(
      "replaceProtoVars --- \n"
      "   {}\n"
      "  -- ref_vars: \n"
      "   {}\n"
      "  -- END --",
      cfg_map, ref_vars);

  for (auto& kv : cfg_map) {
    const auto& k = kv.first;
    auto& v = kv.second;
#if CONFIG_HELPERS_DEBUG
    logger::trace("At: {} = {} | type: {}", k, v, v->type);
#endif
    if (v->type == config::types::Type::kVar) {
      auto v_var = dynamic_pointer_cast<config::types::ConfigVar>(v);
      // Pull the value from the reference vars and add it to the structure.
      if (!ref_vars.contains(v_var->name)) {
        THROW_EXCEPTION(config::UndefinedReferenceVarException,
                        "Attempting to replace '{}' with undefined var: '{}' at {}.", k,
                        v_var->name, v->loc());
      }
      cfg_map[k] = ref_vars.at(v_var->name);
    } else if (v->type == config::types::Type::kString) {
      auto v_value = dynamic_pointer_cast<config::types::ConfigValue>(v);

      // Before doing any of this, maybe check if there is a "$" in v_value->value, otherwise, no
      // sense in doing this loop.
      const auto var_pos = v_value->value.find('$');
      if (var_pos == std::string::npos) {
        // No variables. Let's skip.
        logger::debug("No variables in {}. Skipping...", v);
        continue;
      }

      auto out = v_value->value;
      // Find instances of 'ref_vars' in 'v' and replace.
      for (auto& rkv : ref_vars) {
        const auto& rk = rkv.first;
        auto rv = dynamic_pointer_cast<config::types::ConfigValue>(rkv.second);

        logger::debug("v: {}, rk: {}, rv: {}", out, rk, rkv.second);
        // Strip off any leading or trailing quotes from the replacement value. If the replacement
        // value is not a string, this is a no-op.
        const auto replacement = utils::trim(rv->value, "\\\"");
        out = std::regex_replace(out, std::regex("\\" + rk), replacement);
        // Turn the $VAR version into ${VAR} in case that is used within a string as well. Throw
        // in escape characters as this will be used in a regular expression.
        const auto bracket_var = std::regex_replace(rk, std::regex("\\$(.+)"), "\\$\\{$1\\}");
        logger::debug("v: {}, rk: {}, rv: {}", out, bracket_var, rkv.second);
        out = std::regex_replace(out, std::regex(bracket_var), replacement);
        logger::debug("out: {}", out);
      }
      // Replace the existing value with the new value.
      auto new_value = std::make_shared<config::types::ConfigValue>(out, v->type);
      new_value->line = v->line;
      new_value->source = v->source;
      cfg_map[k] = std::move(new_value);
    } else if (config::helpers::isStructLike(v)) {
      logger::debug("At '{}', found {}", k, v->type);
      // Recurse deeper into the structure in order to replace more variables.
      replaceProtoVar(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data, ref_vars);
    }
  }

  logger::trace(
      "~~~~~ Result ~~~~~\n"
      "   {}\n"
      "===== RPV DONE =====",
      cfg_map);
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
      THROW_EXCEPTION(config::InvalidTypeException,
                      "Expected value at '{}' to be a struct-like object, but got {} type instead.",
                      rejoined, content->at(key)->type);
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

inline auto unflatten(const std::span<std::string> keys, const config::types::CfgMap& cfg)
    -> config::types::CfgMap {
  if (keys.empty()) {
    return cfg;
  }

  auto new_struct = std::make_shared<config::types::ConfigStruct>(keys.back(), keys.size() - 1);
  new_struct->data = cfg;
  return unflatten(keys.subspan(0, keys.size() - 1), {{keys.back(), new_struct}});
}

/// \brief Turns a flat key/value pair into a nested structure
/// \param[in] flat_key - The dot-separated key
/// \param[in/out] cfg - The root of the existing data structure
/// \param[in] depth - The current depth level of the data structure
inline void unflatten(const std::string& flat_key, config::types::CfgMap& cfg,
                      std::size_t depth = 0) {
  // Split off the first element of the flat key
  const auto [head, tail] = utils::splitHead(flat_key);

  if (tail.empty()) {
    // If there's no tail, then nothing to do.
    logger::debug("At last key. Terminating...");
    return;
  }

  // There are two possible options: the key exists in the current map or it does not. Either way,
  // we need a pointer to the map.
  config::types::CfgMap* next_cfg = nullptr;
  if (cfg.contains(head)) {
    logger::trace("Found key '{}'", head);
    // Get this element, and find the internal data and assign it to our pointer.
    auto v = cfg[head];
    if (!config::helpers::isStructLike(v)) {
      THROW_EXCEPTION(std::runtime_error,
                      "In unflatten, expected {} to be struct-like, but found {} instead.", head,
                      v->type);
    }
    next_cfg = &(dynamic_pointer_cast<config::types::ConfigStructLike>(v)->data);
  } else {
    // The key doesn't exist in our map. We need to create a new struct and add it to the map.
    logger::debug("Creating key '{}'", head);
    auto new_struct = std::make_shared<config::types::ConfigStruct>(head, depth);
    cfg[head] = new_struct;
    // Extract the map from our new struct and assign its address to our pointer.
    next_cfg = &(new_struct->data);
  }

  // Move the value to the new cfg (using just the 'tail' as the key).
  logger::trace("Moving value from '{}' to '{}'", flat_key, tail);
  (*next_cfg)[tail] = std::move(cfg[flat_key]);
  // And remove the value from the existing config
  cfg.erase(flat_key);

  // Step a layer deeper with any remaining tail.
  unflatten(tail, *next_cfg, depth + 1);
}

inline void cleanupConfig(config::types::CfgMap& cfg, std::size_t depth = 0) {
  std::vector<std::remove_reference_t<decltype(cfg)>::key_type> to_erase{};
  for (auto& kv : cfg) {
    if (kv.second->type == config::types::Type::kStruct ||
        kv.second->type == config::types::Type::kStructInProto) {
      auto s = dynamic_pointer_cast<config::types::ConfigStruct>(kv.second);
      s->depth = depth;
      cleanupConfig(s->data, depth + 1);
      if (s->data.empty()) {
        logger::debug(" !!! Removing {} !!!", kv.first);
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
