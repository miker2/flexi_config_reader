#include <fmt/format.h>
#include <fmt/ranges.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <range/v3/view/drop_last.hpp>
#include <string>
#include <vector>

#include "config_actions.h"
#include "config_classes.h"
#include "config_exceptions.h"
#include "config_grammar.h"
#include "config_helpers.h"
#include "logger.h"
#include "utils.h"

class ConfigReader {
 public:
  ConfigReader() = default;
  ~ConfigReader() = default;

  auto parse(const std::filesystem::path& cfg_filename) -> bool;

  void dump() const;

  template <typename T>
  auto getValue(const std::string& name) -> T;

  template <typename T>
  void getValue(const std::string& name, T& value);

 private:
  void convert(const std::string& value_str, float& value) const;
  void convert(const std::string& value_str, double& value) const;
  void convert(const std::string& value_str, int& value) const;
  void convert(const std::string& value_str, int64_t& value) const;
  void convert(const std::string& value_str, bool& value) const;
  void convert(const std::string& value_str, std::string& value) const;

  // TODO: This needs to be modified. More info needs to be provided to this function, as it appears
  // to work for single values that aren't read as a LIST type.
  template <typename T>
  void convert(const std::string& value_str, std::vector<T>& value);

  // TODO: This needs to be modified. More info needs to be provided to this function, as it appears
  // to work for single values that aren't read as a LIST type.
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
                         const config::types::RefMap& ref_vars);

  void resolveVarRefs(const config::types::CfgMap& root, config::types::CfgMap& sub_tree,
                      const std::string& parent_key = "");

  config::ActionData out_;
  std::map<std::string, std::shared_ptr<config::types::ConfigProto>> protos_{};

  // All of the config data!
  config::types::CfgMap cfg_data_;
};

template <typename T>
void ConfigReader::getValue(const std::string& name, T& value) {
  // Split the key into parts
  const auto keys = utils::split(name, '.');

  const auto struct_like = config::helpers::getNestedConfig(cfg_data_, keys);

  // Special handling for the case where 'name' contains a single key (i.e is not a flat key)
  const auto cfg_val =
      (struct_like != nullptr) ? struct_like->data.at(keys.back()) : cfg_data_.at(keys.back());

  // I'm not sure this really belongs here, but unless we pass more info to `convert` we can't check
  // there.
  if (config::accepts_list_v<T> && cfg_val->type != config::types::Type::kList) {
    throw std::runtime_error(
        fmt::format("Expected '{}' to contain a list, but is of type {}", name, cfg_val->type));
  }

  const auto value_str = dynamic_pointer_cast<config::types::ConfigValue>(cfg_val)->value;
  convert(value_str, value);
  logger::debug(" -- Type is {}", typeid(T).name());
}

template <typename T>
auto ConfigReader::getValue(const std::string& name) -> T {
  T value;
  getValue(name, value);
  return value;
}

template <typename T>
void ConfigReader::convert(const std::string& value_str, std::vector<T>& value) {
  auto entries = utils::split(utils::trim(value_str, "[] \t\n\r"), ',');
  // Clear the vector before inserting new elements. We only want to return the elements in the
  // provided list, not append new ones.
  value.clear();
  std::transform(entries.begin(), entries.end(), std::back_inserter(value), [this](auto s) {
    T out{};
    // Remove any leading/trailing whitespace from each list element.
    convert(utils::trim(s), out);
    return out;
  });
}

template <typename T, std::size_t N>
void ConfigReader::convert(const std::string& value_str, std::array<T, N>& value) {
  auto entries = utils::split(utils::trim(value_str, "[] \t\n\r"), ',');
  // Try to do the right thing, even if there aren't the right number of elements. This will allow
  // the use to catch the exception and use the results if desired.
  auto end_it = entries.size() <= N ? std::end(entries) : std::next(std::begin(entries), N);
  std::transform(entries.begin(), end_it, value.begin(), [this](auto s) {
    T out{};
    // Remove any leading/trailing whitespace from each list element.
    convert(utils::trim(s), out);
    return out;
  });
  if (entries.size() != N) {
    throw std::runtime_error(fmt::format("Expected {} entries in '{}', but found {}!", N,
                                         utils::trim(value_str), entries.size()));
  }
}
