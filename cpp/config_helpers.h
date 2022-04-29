#pragma once

#include <memory>
#include <span>

#include "config_classes.h"

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

auto isStructLike(const types::BasePtr& el) -> bool;

// Three cases to check for:
//   - Both are dictionaries     - This is okay
//   - Neither are dictionaries  - This is bad - even if the same value, we don't allow duplicates
//   - Only ones is a dictionary - Also bad. We can't handle this one
auto checkForErrors(const types::CfgMap& cfg1, const types::CfgMap& cfg2, const std::string& key)
    -> bool;

/* Merge dictionaries recursively and keep all nested keys combined between the two dictionaries.
 * Any key/value pairs that already exist in the leaves of cfg1 will be overwritten by the same
 * key/value pairs from cfg2. */
auto mergeNestedMaps(const types::CfgMap& cfg1, const types::CfgMap& cfg2) -> types::CfgMap;

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

void resolveVarRefs(const types::CfgMap& root, types::CfgMap& sub_tree,
                    const std::string& parent_key = "");

auto unflatten(const std::span<std::string> keys, const types::CfgMap& cfg) -> types::CfgMap;

/// \brief Turns a flat key/value pair into a nested structure
/// \param[in] flat_key - The dot-separated key
/// \param[in/out] cfg - The root of the existing data structure
/// \param[in] depth - The current depth level of the data structure
void unflatten(const std::string& flat_key, types::CfgMap& cfg, std::size_t depth = 0);

void cleanupConfig(types::CfgMap& cfg, std::size_t depth = 0);

}  // namespace config::helpers
