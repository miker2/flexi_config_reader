#include <fmt/format.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/view/drop_last.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/set_algorithm.hpp>
#include <regex>
#include <span>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/exceptions.h"
#include "flexi_cfg/config/helpers.h"
#include "flexi_cfg/config/parser-internal.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/math/actions.h"
#include "flexi_cfg/utils.h"

namespace {
constexpr bool CONFIG_HELPERS_DEBUG{false};
}

namespace flexi_cfg::config::helpers {

auto isStructLike(const types::BasePtr& el) -> bool {
  // TODO(miker2): Change this to be a `dynamic_pointer_cast`. As is, it's a bit of a maintenance
  // burden.
  return el->type == types::Type::kStruct || el->type == types::Type::kStructInProto ||
         el->type == types::Type::kProto || el->type == types::Type::kReference;
}

// Three cases to check for:
//   - Both are dictionaries     - This is okay
//   - Neither are dictionaries  - This is bad - even if the same value, we don't allow duplicates
//   - Only ones is a dictionary - Also bad. We can't handle this one
auto checkForErrors(const types::CfgMap& cfg1, const types::CfgMap& cfg2, const std::string& key)
    -> bool {
  const auto dict_count =
      static_cast<int>(isStructLike(cfg1.at(key))) + static_cast<int>(isStructLike(cfg2.at(key)));
  if (dict_count == 0) {  // Neither is a dictionary. We don't support duplicate keys
    THROW_EXCEPTION(DuplicateKeyException, "Duplicate key '{}' found at {} and {}!", key,
                    cfg1.at(key)->loc(), cfg2.at(key)->loc());
    // print(f"Found duplicate key: '{key}' with values '{dict1[key]}' and '{dict2[key]}'!!!")
  }
  if (dict_count == 1) {  // One dictionary, but not the other, can't merge these
    THROW_EXCEPTION(MismatchKeyException,
                    "Mismatch types for key '{}' found at {} and {}! Both keys must point to "
                    "structs, can't merge these.",
                    key, cfg1.at(key)->loc(), cfg2.at(key)->loc());
  }
  if (cfg1.at(key)->type != cfg2.at(key)->type) {
    THROW_EXCEPTION(MismatchTypeException,
                    "Types at key '{}' must match. cfg1 is '{}', cfg2 is '{}'.", key,
                    cfg1.at(key)->type, cfg2.at(key)->type);
  }
  return dict_count == 2;  // All good if both are dictionaries (for now);
}

/* Merge dictionaries recursively and keep all nested keys combined between the two dictionaries.
 * Any key/value pairs that already exist in the leaves of cfg1 will be overwritten by the same
 * key/value pairs from cfg2. */
auto mergeNestedMaps(const types::CfgMap& cfg1, const types::CfgMap& cfg2) -> types::CfgMap {
  const auto common_keys =
      ranges::views::set_intersection(cfg1 | ranges::views::keys, cfg2 | ranges::views::keys);

  for (const auto& k : common_keys) {
    checkForErrors(cfg1, cfg2, k);
  }

  // This over-writes the top level keys in dict1 with dict2. If there are
  // nested dictionaries, we need to handle these appropriately.
  types::CfgMap cfg_out{};
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
        dynamic_pointer_cast<types::ConfigStructLike>(cfg_out[key])->data =
            mergeNestedMaps(dynamic_pointer_cast<types::ConfigStructLike>(cfg1.at(key))->data,
                            dynamic_pointer_cast<types::ConfigStructLike>(cfg2.at(key))->data);
      }
    }
  }

  return cfg_out;
}

auto structFromReference(std::shared_ptr<types::ConfigReference>& ref,
                         const std::shared_ptr<types::ConfigProto>& proto)
    -> std::shared_ptr<types::ConfigStruct> {
  // NOTE: This will not fill in any of the reference variables. It will only generate a struct from
  // an existing reference and proto object.

  // First, create the new struct based on the reference data.
  auto struct_out = std::make_shared<types::ConfigStruct>(ref->name, ref->depth);
  if (CONFIG_HELPERS_DEBUG) {
    logger::debug("New struct: \n{}", struct_out);
  }

  // Next, move the data from the reference to the struct:
  struct_out->data.merge(ref->data);
  if (CONFIG_HELPERS_DEBUG) {
    logger::debug("Data added: \n{}", struct_out);
  }

  // Next, we need to copy the values from the proto into the reference (need a deep-copy here so
  // that modifying the `std::shared_ptr` objects copied into the reference don't affect those in
  // the proto.
  for (const auto& el : proto->data) {
    if (struct_out->data.contains(el.first) && proto->data.contains(el.first)) {
      checkForErrors(struct_out->data, proto->data, el.first);
    }
    // types::BasePtr value = el.second->clone();
    struct_out->data[el.first] = el.second->clone();
  }
  return struct_out;
}

auto replaceVarInStr(std::string input, const types::RefMap& ref_vars)
    -> std::optional<std::string> {
  // Before doing any of this, maybe check if there is a "$" in v_value->value, otherwise, no
  // sense in doing this loop.
  const auto var_pos = input.find('$');
  if (var_pos == std::string::npos) {
    // No variables. Let's skip.
    logger::debug("No variables in '{}'. Skipping...", input);
    return std::nullopt;
  }

  auto out = std::move(input);
  // Find instances of 'ref_vars' in 'input' and replace.
  for (const auto& rkv : ref_vars) {
    const auto& rk = rkv.first;
    auto rv = dynamic_pointer_cast<types::ConfigValue>(rkv.second);
    // Only continue if the k/v pair contains a value (i.e. not a kValueLookup or kVar)
    if (rv == nullptr) {
      logger::trace(" -- rK: {} is of type {} and does not contain a string. Skipping...", rk,
                    rkv.second->type);
      continue;
    }
    logger::debug("v: {}, rk: {}, rv: {}", out, rk, rkv.second);

    // Strip off any leading or trailing quotes from the replacement value. If the replacement
    // value is not a string, this is a no-op.
    const auto replacement = utils::trim(rv->value, "\\\"");
    // Look for `rk` (escape leading '$') in `out` and replace them with 'replacement'
    out = std::regex_replace(out, std::regex(std::string("\\").append(rk)), replacement);
    // Turn the $VAR version into ${VAR} in case that is used within a string as well. Throw
    // in escape characters as this will be used in a regular expression.
    const auto bracket_var = std::regex_replace(rk, std::regex("\\$(.+)"), R"(\$\{$1\})");
    logger::debug("v: {}, rk: {}, rv: {}", out, bracket_var, rkv.second);
    out = std::regex_replace(out, std::regex(bracket_var), replacement);
    logger::debug("out: {}", out);
  }
  return out;
}

/// \brief Finds all uses of 'ConfigVar' in the contents of a proto and replaces them
/// \param[in/out] cfg_map - Contents of a proto
/// \param[in] ref_vars - All of the available 'ConfigVar's in the reference
void replaceProtoVar(types::CfgMap& cfg_map, const types::RefMap& ref_vars) {
  // TODO(miker2): This is probably the *most* complex part of the entire config resolution
  // algorithm. Perhaps revisit to see if it can be simplified/clarified at all.
  logger::trace(
      "replaceProtoVars --- \n"
      "   {}\n"
      "  -- ref_vars: \n"
      "   {}\n"
      "  -- END --",
      cfg_map, ref_vars);

  auto str_contains_var = [](const std::string& str) {
    // Use the existing VAR grammar rule to check if any variables exist
    peg::memory_input in(str, "contains VAR?");
    config::ActionData state;
    // NOTE: 'peg::until' will consume everything until the rule 'config::VAR' matches. If
    // 'config::VAR' never matches, then 'parse' will return false, otherwise true
    const auto ret = internal::parseCore<peg::until<config::VAR>, config::action>(in, state);
    return ret;
  };

  for (auto& kv : cfg_map) {
    const auto& k = kv.first;
    auto& v = kv.second;
    if (CONFIG_HELPERS_DEBUG) {
      logger::trace("At: {} = {} | type: {}", k, v, v->type);
    }

    /// \brief A helper function for grabbing the correct ref_var from the map and returning it
    auto replace_var = [&ref_vars = std::as_const(ref_vars),
                        &k = std::as_const(k)](const types::BasePtr& v) {
      auto v_var = dynamic_pointer_cast<types::ConfigVar>(v);
      // Pull the value from the reference vars and add it to the structure.
      if (!ref_vars.contains(v_var->name)) {
        THROW_EXCEPTION(UndefinedReferenceVarException,
                        "Attempting to replace '{}' with undefined var: '{}' at {}.", k,
                        v_var->name, v->loc());
      }
      return ref_vars.at(v_var->name);
    };

    /// \brief A helper function for replacing VAR elements within a string
    auto replace_var_in_str = [&ref_vars = std::as_const(ref_vars)](const types::BasePtr& v) {
      auto v_value = dynamic_pointer_cast<types::ConfigValue>(v);

      auto out = replaceVarInStr(v_value->value, ref_vars);
      if (!out.has_value()) {
        // There were no references in the string, so just return the original object;
        return v;
      }
      // Replace the existing value with the new value.
      std::shared_ptr<types::ConfigBase> new_value =
          std::make_shared<types::ConfigValue>(out.value(), v->type);

      new_value->line = v->line;
      new_value->source = v->source;
      return new_value;
    };

    if (v->type == types::Type::kVar) {
      cfg_map[k] = replace_var(v);
    } else if (v->type == types::Type::kString) {
      cfg_map[k] = replace_var_in_str(v);
    } else if (v->type == types::Type::kList) {
      auto v_list = dynamic_pointer_cast<types::ConfigList>(v);
      logger::trace("Resolving references in list: {}", v_list);
      for (auto& e : v_list->data) {
        logger::trace("Element type: {}, data: {}", e->type, e);
        if (e->type == types::Type::kVar) {
          e = replace_var(e);
        } else if (e->type == types::Type::kString) {
          e = replace_var_in_str(e);
        }
        if (!listElementValid(v_list, e->type)) {
          THROW_EXCEPTION(InvalidTypeException,
                          "While resolving a reference in {} ({}), encountered an incorrect type. "
                          "Expected {}, but found {}",
                          k, v_list->loc(), v_list->list_element_type, e->type);
        }
        // If this is any other type, we're going to skip it
      }
      logger::trace("Resolved list: {}", v_list);
    } else if (v->type == types::Type::kExpression) {
      auto expression = dynamic_pointer_cast<types::ConfigExpression>(v);

      auto out = replaceVarInStr(expression->value, ref_vars);
      if (!out.has_value()) {
        continue;
      }
      // If anything has been replaced, we need to recreate the entire expression using the grammar
      // (in order to identify any ValueLookup objects in the expression now that the Var objects
      // have been resolved.
      config::ActionData state;
      peg::memory_input input(out.value(), k);
      internal::parseCore<EXPRESSION, config::action>(input, state);

      state.obj_res->line = v->line;
      state.obj_res->source = v->source;
      auto contains_var = str_contains_var(out.value());
      logger::debug("{} has var? {}", out.value(), contains_var);
      if (contains_var) {
        THROW_EXCEPTION(
            InvalidConfigException,
            "Key: '{}' of type '{}' (at {}) contains unresolved VARs: '{}'. Did reference '{}' "
            "fail to define all variables?",
            k, v->type, v->loc(), out.value(), ref_vars.at("$PARENT"));
      }
      cfg_map[k] = std::move(state.obj_res);
    } else if (v->type == types::Type::kValueLookup) {
      logger::debug("Key: {}, checking {} for vars.", k, v);
      auto v_val_lookup = dynamic_pointer_cast<types::ConfigValueLookup>(v);

      auto out = replaceVarInStr(v_val_lookup->var(), ref_vars);
      if (!out.has_value()) {
        continue;
      }
      // Replace the existing value lookup with the new value lookup
      auto new_val_lookup = std::make_shared<types::ConfigValueLookup>(out.value());
      new_val_lookup->line = v->line;
      new_val_lookup->source = v->source;
      cfg_map[k] = std::move(new_val_lookup);
    } else if (isStructLike(v)) {
      logger::debug("At '{}', found {}", k, v->type);
      // Recurse deeper into the structure in order to replace more variables.
      replaceProtoVar(dynamic_pointer_cast<types::ConfigStructLike>(v)->data, ref_vars);
    }
  }

  logger::trace(
      "~~~~~ Result ~~~~~\n"
      "   {}\n"
      "===== RPV DONE =====",
      cfg_map);
}

auto getNestedConfig(const types::CfgMap& cfg, const std::vector<std::string>& keys)
    -> std::shared_ptr<types::ConfigStructLike> {
  // Keep track of the re-joined keys (from the front to the back) as we recurse into the map in
  // order to make error messages more useful.
  std::string rejoined{};
  // Start with the base config tree and work our way down through the keys.
  const auto* content = &cfg;
  // Walk through each part of the flat key (except the last one, as it will be the one that
  // contains the data.
  std::shared_ptr<types::ConfigStructLike> struct_like;
  for (const auto& key : keys | ranges::views::drop_last(1)) {
    if (!content->contains(key)) {
      THROW_EXCEPTION(InvalidKeyException, "Unable to find '{}' in '{}'!", key, rejoined);
    }

    rejoined = utils::makeName(rejoined, key);

    struct_like = dynamic_pointer_cast<types::ConfigStructLike>(content->at(key));
    // If the dynamic_pointer_cast fails, then we can't continue with the loop and whatever is
    // returned by 'content->at(key)' doesn't have any content of it's own.
    if (struct_like == nullptr) {
      THROW_EXCEPTION(InvalidTypeException,
                      "Expected value at '{}' to be a struct-like object, but got {} type instead.",
                      rejoined, content->at(key)->type);
    }
    // Pull out the contents of the struct-like and move on to the next iteration.
    content = &(struct_like->data);
  }

  return struct_like;
}

auto getNestedConfig(const types::CfgMap& cfg, const std::string& flat_key)
    -> std::shared_ptr<types::ConfigStructLike> {
  const auto keys = utils::split(flat_key, '.');
  return getNestedConfig(cfg, keys);
}

auto getConfigValue(const types::CfgMap& cfg, const std::vector<std::string>& keys)
    -> types::BasePtr {
  // Get the struct-like object containing the last key of the ConfigValueLookup:
  const auto struct_like = getNestedConfig(cfg, keys);

  // Special handling for the case where 'keys' only has one entry:
  // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
  const auto& cfg_tail = (struct_like != nullptr) ? struct_like->data : cfg;

  if (!cfg_tail.contains(keys.back())) {
    auto keys_minus_tail = keys;
    keys_minus_tail.pop_back();
    THROW_EXCEPTION(InvalidKeyException, "Unable to find '{}' in '{}'!", keys.back(),
                    utils::join(keys_minus_tail, "."));
  }

  // Extract the value from the final CfgMap object using the final key.
  return cfg_tail.at(keys.back());
}

auto getConfigValue(const types::CfgMap& cfg, const std::shared_ptr<types::ConfigValueLookup>& var)
    -> types::BasePtr {
  return getConfigValue(cfg, var->keys);
}

/// \brief Handles the resolution of kValueLookup objects
/// \param root - The root of the config tree
/// \param src_key - The key from which the kValueLookup originates
/// \param src - The kValueLookup object itself
auto resolveVarRefs(const types::CfgMap& root, const std::string& src_key,
                    const std::shared_ptr<types::ConfigBase>& src)
    -> std::shared_ptr<types::ConfigBase> {
  std::vector<std::string> refs = {src_key};
  auto value = src;
  // Follow the kValueLookup objects until:
  //  (a) A terminal value is found, upon which the value is used.
  //  (b) A cycle is found, upon which an exception is thrown.
  do {
    auto kv_lookup = dynamic_pointer_cast<types::ConfigValueLookup>(value);
    if (utils::contains(refs, kv_lookup->var())) {
      // This key has already been traversed/referenced. A cycle has been found!
      THROW_EXCEPTION(CyclicReferenceException,
                      "For {}, found a cyclic reference when trying to resolve {}.\n  "
                      "Reference chain: [{}]\n",
                      src_key, src, fmt::join(refs, " -> "));
    }
    // Get the new value based on the kValueLookup object.
    value = getConfigValue(root, kv_lookup);
    logger::trace("{} points to {}", kv_lookup, value);
    // Add this key to the list of references/dependencies
    refs.emplace_back(kv_lookup->var());
    logger::trace("refs: [{}]", fmt::join(refs, " -> "));
  } while (value->type == types::Type::kValueLookup);
  logger::debug("{} contains instance of ValueLookup: {}. Has value={}", src_key, src, value);
  // If we've gotten here, then we can return the current 'value'!
  return value;
}

void resolveVarRefs(const types::CfgMap& root, types::CfgMap& sub_tree,
                    const std::string& parent_key) {
  auto resolve_expression_vars = [&root](std::shared_ptr<types::ConfigExpression>& expression,
                                         const std::string& src_key) {
    assert(expression.get() != nullptr);
    logger::trace("Calling resolve_expression_vars with expression={}, src_key={}", expression,
                  src_key);
    for (auto& kvl : expression->value_lookups) {
      if (kvl.second->type == types::Type::kNumber) {
        // We may have already resolved this variable, so no need to do anything
        continue;
      }
      logger::trace("Attempting to resolve {} = {}", kvl.first, kvl.second);
      auto value = resolveVarRefs(root, src_key, kvl.second);
      if (value->type == types::Type::kExpression) {
        logger::debug("Found sub expression '{}' when trying to evaluate '{}'.", kvl.first,
                      src_key);
        // Follow the trail!
        auto sub_expression = dynamic_pointer_cast<types::ConfigExpression>(value);
        value = evaluateExpression(sub_expression, src_key);
      }
      if (value->type != types::Type::kNumber) {
        THROW_EXCEPTION(InvalidTypeException,
                        "All key/value references in expressions must be of numeric type!\n"
                        "When looking up '{}' in '{} = {}' at {} found '{}' of type {}.",
                        kvl.first, src_key, expression, expression->loc(), value, value->type);
      }
      expression->value_lookups[kvl.first] = value;
    }
  };

  for (const auto& kv : sub_tree) {
    const auto src_key = utils::makeName(parent_key, kv.first);
    if (kv.second && kv.second->type == types::Type::kValueLookup) {
      // Add the source key to the reference list (if we ever get back to this key, it's a failure).
      logger::trace("For {}, found {} (type={}).", src_key, kv.second, kv.second->type);
      sub_tree[kv.first] = resolveVarRefs(root, src_key, kv.second);
    } else if (kv.second && kv.second->type == types::Type::kExpression) {
      auto expression = dynamic_pointer_cast<types::ConfigExpression>(kv.second);
      resolve_expression_vars(expression, src_key);
    } else if (kv.second && kv.second->type == types::Type::kList) {
      // Check the elements of the list to see if it contains any kValueLookup objects
      auto list = dynamic_pointer_cast<types::ConfigList>(kv.second);
      for (auto& el : list->data) {
        if (el->type == types::Type::kValueLookup) {
          logger::trace("Found {} in {}.{} which is a {}", el->type, parent_key, kv.first,
                        kv.second->type);
          const auto el_resolved = resolveVarRefs(root, src_key, el);
          // Since this list contains a ValueLookup, we need to check if it is consistent (i.e. all
          // elements in the list are of the same type)
          if (!listElementValid(list, el_resolved->type)) {
            THROW_EXCEPTION(InvalidTypeException,
                            "While resolving a key/value reference ($({})) in {} (type: {}), "
                            "encountered an incorrect type. Expected {}, but found {}",
                            dynamic_pointer_cast<types::ConfigValueLookup>(el)->var(), src_key,
                            kv.second->type, list->list_element_type, el_resolved->type);
          }
          el = el_resolved;
        } else if (el->type == types::Type::kExpression) {
          logger::trace("Found {} ({}) in {}", el->type, el, list);
          auto expression = dynamic_pointer_cast<types::ConfigExpression>(el);
          resolve_expression_vars(expression, src_key);
        }
      }
    } else if (isStructLike(kv.second)) {
      resolveVarRefs(root, dynamic_pointer_cast<types::ConfigStructLike>(kv.second)->data, src_key);
    }
  }
}

auto evaluateExpression(std::shared_ptr<types::ConfigExpression>& expression,
                        const std::string& key) -> std::shared_ptr<types::ConfigValue> {
  math::ActionData math;
  // Fill in math.var_ref_map;
  for (const auto& var_ref : expression->value_lookups) {
    if (var_ref.second->type != types::Type::kNumber) {
      logger::critical("{} is not a number (type={})!", var_ref.first, var_ref.second->type);
      THROW_EXCEPTION(
          InvalidTypeException,
          "When trying to evaluate expression '{} = {}' at {}, found {} of type={}, but "
          "expected {}",
          key, expression, expression->loc(), var_ref.first, var_ref.second->type,
          types::Type::kNumber);
    }
    math.var_ref_map[var_ref.first] =
        std::stod(dynamic_pointer_cast<types::ConfigValue>(var_ref.second)->value);
  }
  peg::memory_input input(expression->value, key);
  internal::parseCore<peg::seq<config::Eo, math::expression, config::Ec>, math::action>(input,
                                                                                        math);
  return std::make_shared<types::ConfigValue>(std::to_string(math.res), types::Type::kNumber,
                                              math.res);
}

void evaluateExpressions(types::CfgMap& cfg, const std::string& parent_key) {
  for (auto& kv : cfg) {
    const auto key = utils::makeName(parent_key, kv.first);
    if (kv.second && kv.second->type == types::Type::kExpression) {
      // Evaluate expression
      logger::debug("Evaluating expression {} = {}", key, kv.second);
      auto expression = dynamic_pointer_cast<types::ConfigExpression>(kv.second);
      cfg[kv.first] = evaluateExpression(expression);
    } else if (kv.second && kv.second->type == types::Type::kList) {
      auto list = dynamic_pointer_cast<types::ConfigList>(kv.second);
      for (auto& el : list->data) {
        if (el->type == types::Type::kExpression) {
          logger::debug("Found a list element that contains an expression: {}!", el);
          auto expression = dynamic_pointer_cast<types::ConfigExpression>(el);
          el = evaluateExpression(expression);
          if (!listElementValid(list, el->type)) {
            logger::critical("Invalid list element type! Expected {}, but got {}",
                             list->list_element_type, el->type);
            THROW_EXCEPTION(InvalidTypeException,
                            "Resulting element of list '{}' has invalid type {}", list, el->type);
          }
        }
      }
    } else if (isStructLike(kv.second)) {
      evaluateExpressions(dynamic_pointer_cast<types::ConfigStructLike>(kv.second)->data, key);
    }
  }
}

auto unflatten(const std::span<std::string> keys, const types::CfgMap& cfg) -> types::CfgMap {
  if (keys.empty()) {
    return cfg;
  }

  auto new_struct = std::make_shared<types::ConfigStruct>(keys.back(), keys.size() - 1);
  new_struct->data = cfg;
  return unflatten(keys.subspan(0, keys.size() - 1), {{keys.back(), new_struct}});
}

/// \brief Turns a flat key/value pair into a nested structure
/// \param[in] flat_key - The dot-separated key
/// \param[in/out] cfg - The root of the existing data structure
/// \param[in] depth - The current depth level of the data structure
void unflatten(const std::string& flat_key, types::CfgMap& cfg, std::size_t depth) {
  // Split off the first element of the flat key
  const auto [head, tail] = utils::splitHead(flat_key);

  if (tail.empty()) {
    // If there's no tail, then nothing to do.
    logger::debug("At last key. Terminating...");
    return;
  }

  // There are two possible options: the key exists in the current map or it does not. Either way,
  // we need a pointer to the map.
  types::CfgMap* next_cfg = nullptr;
  if (cfg.contains(head)) {
    logger::trace("Found key '{}'", head);
    // Get this element, and find the internal data and assign it to our pointer.
    auto v = cfg[head];
    if (!isStructLike(v)) {
      THROW_EXCEPTION(InvalidTypeException,
                      "In unflatten, expected {} to be struct-like, but found {} instead.", head,
                      v->type);
    }
    next_cfg = &(dynamic_pointer_cast<types::ConfigStructLike>(v)->data);
  } else {
    // The key doesn't exist in our map. We need to create a new struct and add it to the map.
    logger::debug("Creating key '{}'", head);
    auto new_struct = std::make_shared<types::ConfigStruct>(head, depth);
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

void cleanupConfig(types::CfgMap& cfg, std::size_t depth) {
  std::vector<std::remove_reference_t<decltype(cfg)>::key_type> to_erase{};
  for (auto& kv : cfg) {
    if (kv.second->type == types::Type::kStruct || kv.second->type == types::Type::kStructInProto) {
      auto s = dynamic_pointer_cast<types::ConfigStruct>(kv.second);
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

auto listElementValid(const std::shared_ptr<types::ConfigList>& list, types::Type type) -> bool {
  bool valid = true;
  if (type == types::Type::kVar || type == types::Type::kValueLookup ||
      type == types::Type::kExpression) {
    // This is a VAR or VALUE_LOOKUP, so we'll just continue. It's okay to mix these. We'll resolve
    // them later.
  } else if (list->list_element_type == types::Type::kUnknown) {
    list->list_element_type = type;
  } else if (list->list_element_type != type) {
    valid = false;
  }
  return valid;
}

}  // namespace flexi_cfg::config::helpers
