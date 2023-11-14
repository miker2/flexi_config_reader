#pragma once

#include <memory>
#include <span>

#include "flexi_cfg/config/classes.h"

namespace flexi_cfg::config {
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
}  // namespace flexi_cfg::config

namespace flexi_cfg::config::helpers {

auto isStructLike(const types::BasePtr& el) -> bool;

// Three cases to check for:
//   - Both are dictionaries     - This is okay
//   - Neither are dictionaries  - This is bad - even if the same value, we don't allow duplicates
//   - Only ones is a dictionary - Also bad. We can't handle this one
auto checkForErrors(const types::CfgMap& cfg1, const types::CfgMap& cfg2, const std::string& key)
    -> bool;

/// @brief Merge two dictionaries recursively, prioritizing rhs over lhs in the event of a conflict.
/// @param lhs Base dictionary to merge into
/// @param rhs Overriding dictionary
/// @param is_overlay If true, then the merge will be done in an overlay fashion, meaning that all keys in rhs must exist in lhs with the same value type.
void mergeLeft(types::CfgMap& lhs, const types::CfgMap& rhs, const bool is_overlay = false);

/* Merge dictionaries recursively and keep all nested keys combined between the two dictionaries.
 * Any key/value pairs that already exist in the leaves of cfg1 will be overwritten by the same
 * key/value pairs from cfg2. */
auto mergeNestedMaps(const types::CfgMap& cfg1, const types::CfgMap& cfg2) -> types::CfgMap;

/// @brief Compare two maps recursively
/// @param lhs First map
/// @param rhs Second map
/// @return true if the two maps are identical
auto compareNestedMaps(const types::CfgMap& lhs, const types::CfgMap& rhs) -> bool;

auto structFromReference(std::shared_ptr<types::ConfigReference>& ref,
                         const std::shared_ptr<types::ConfigProto>& proto)
    -> std::shared_ptr<types::ConfigStruct>;

auto replaceVarInStr(std::string input, const types::RefMap& ref_vars)
    -> std::optional<std::string>;

/// \brief Finds all uses of 'ConfigVar' in the contents of a proto and replaces them
/// \param[in/out] cfg_map - Contents of a proto
/// \param[in] ref_vars - All of the available 'ConfigVar's in the reference
void replaceProtoVar(types::CfgMap& cfg_map, const types::RefMap& ref_vars);

auto getNestedConfig(const types::CfgMap& cfg, const std::vector<std::string>& keys)
    -> std::shared_ptr<types::ConfigStructLike>;

auto getNestedConfig(const types::CfgMap& cfg, const std::string& flat_key)
    -> std::shared_ptr<types::ConfigStructLike>;

auto getConfigValue(const types::CfgMap& cfg, const std::vector<std::string>& keys)
    -> types::BasePtr;

auto getConfigValue(const types::CfgMap& cfg, const std::shared_ptr<types::ConfigValueLookup>& var)
    -> types::BasePtr;

/// \brief Finds all ValueLookup objects and resolves them
void resolveVarRefs(const types::CfgMap& root, types::CfgMap& sub_tree,
                    const std::string& parent_key = "");

auto evaluateExpression(std::shared_ptr<types::ConfigExpression>& expression,
                        const std::string& key = "unknown") -> std::shared_ptr<types::ConfigValue>;

void evaluateExpressions(types::CfgMap& cfg, const std::string& parent_key = "");

auto unflatten(std::span<std::string> keys, const types::CfgMap& cfg) -> types::CfgMap;

/// \brief Turns a flat key/value pair into a nested structure
/// \param[in] flat_key - The dot-separated key
/// \param[in/out] cfg - The root of the existing data structure
/// \param[in] depth - The current depth level of the data structure
void unflatten(const std::string& flat_key, types::CfgMap& cfg, std::size_t depth = 0);

void cleanupConfig(types::CfgMap& cfg, std::size_t depth = 0);

auto listElementValid(const std::shared_ptr<types::ConfigList>& list, types::Type type) -> bool;

}  // namespace flexi_cfg::config::helpers
