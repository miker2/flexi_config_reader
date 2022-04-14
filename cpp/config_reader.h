#include <fmt/format.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <range/v3/view/drop_last.hpp>
#include <vector>

#include "config_actions.h"
#include "config_classes.h"
#include "config_exceptions.h"
#include "config_grammar.h"
#include "utils.h"

class ConfigReader {
 public:
  ConfigReader() = default;
  ~ConfigReader() = default;

  auto parse(const std::filesystem::path& cfg_filename) -> bool;

  template <typename T>
  auto getValue(const std::string& name) -> T;

 private:
  void convert(const std::string& value_str, float& value) const;
  void convert(const std::string& value_str, double& value) const;
  void convert(const std::string& value_str, int& value) const;
  void convert(const std::string& value_str, int64_t& value) const;
  void convert(const std::string& value_str, bool& value) const;
  void convert(const std::string& value_str, std::string& value) const;

  template <typename T>
  void convert(const std::string& value_str, std::vector<T>& value);

  template <typename T, size_t N>
  void convert(const std::string& value_str, std::array<T, N>& value);

  auto flattenAndFindProtos(const config::types::CfgMap& in, const std::string& base_name,
                            config::types::CfgMap flattened = {}) -> config::types::CfgMap;

  auto mergeNested(const std::vector<config::types::CfgMap>& in) -> config::types::CfgMap;

  /// \brief Remove the protos from merged dictionary
  /// \param[in/out] cfg_map - The top level (resolved) config map
  void stripProtos(config::types::CfgMap& cfg_map);

  /// \brief Walk through CfgMap and find all references. Convert them to structs
  /// \param[in/out] cfg_map
  /// \param[in] base_name - starting point for resolving references
  /// \param[in] ref_vars - map of all reference variables available in the current context
  void resolveReferences(config::types::CfgMap& cfg_map, const std::string& base_name,
                         config::types::RefMap ref_vars);

  void resolveVarRefs(const config::types::CfgMap& root, config::types::CfgMap& sub_tree);

  config::ActionData out_;
  std::map<std::string, std::shared_ptr<config::types::ConfigProto>> protos_{};

  // All of the config data!
  config::types::CfgMap cfg_data_;
};

template <typename T>
auto ConfigReader::getValue(const std::string& name) -> T {
  // Split the key into parts
  const auto parts = utils::split(name, '.');

  // Keep track of the re-joined keys (from the front to the back) as we recurse into the map in
  // order to make error messages more useful.
  std::string rejoined = "";
  // Start with the base config tree and work our way down through the keys.
  auto* content = &cfg_data_;
  // Walk through each part of the flat key (except the last one, as it will be the one that
  // contains the data.
  for (const auto& key : parts | ranges::views::drop_last(1)) {
    rejoined = utils::makeName(rejoined, key);
    // This may be uneccessary error checking (if this is a part of a flat key, then it must be
    // a nested structure), but we check here that this object is a `StructLike` object,
    // otherwise we can't access the `data` member.
    const auto v = dynamic_pointer_cast<config::types::ConfigStructLike>(content->at(key));
    if (v == nullptr) {
      throw config::InvalidTypeException(
          fmt::format("Expected value at '{}' to be a struct-like object, but got {} type instead.",
                      rejoined, content->at(key)->type));
    }
    // Pull out the contents of the struct-like and move on to the next iteration.
    content = &(v->data);
  }

  // Get the actual value. It will need to be converted into something reasonable.
  const auto value = content->at(parts.back());

  const auto value_str = dynamic_pointer_cast<config::types::ConfigValue>(value)->value;
  T ret_val;
  std::cout << " -- Type is " << typeid(T).name() << std::endl;
  convert(value_str, ret_val);
  return ret_val;
}

template <typename T>
void ConfigReader::convert(const std::string& value_str, std::vector<T>& value) {
  std::cout << "Trying to convert a std::vector of type " << typeid(T).name() << std::endl;
  auto entries = utils::split(utils::trim(value_str, "[] \t\n\r"), ',');
  // Remove any leading/trailing whitespace from each list element.
  value.clear();
  std::transform(entries.begin(), entries.end(), std::back_inserter(value), [this](auto s) {
    T out{};
    convert(utils::trim(s), out);
    return out;
  });
}

template <typename T, size_t N>
void ConfigReader::convert(const std::string& value_str, std::array<T, N>& value) {
  auto entries = utils::split(utils::trim(value_str, "[] \t\n\r"), ',');
  // Remove any leading/trailing whitespace from each list element.
  if (entries.size() != N) {
    throw std::runtime_error(
        fmt::format("Expected {} entries in '{}', but found {}!", N, value, entries.size()));
  }
  std::transform(entries.begin(), entries.end(), value.begin(), [this](auto s) {
    T out{};
    convert(utils::trim(s), out);
    return out;
  });
}
