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
#include "config_helpers.h"
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
  const auto keys = utils::split(name, '.');

  const auto struct_like = config::helpers::getNestedConfig(cfg_data_, keys);

  const auto value =
      (struct_like != nullptr) ? struct_like->data.at(keys.back()) : cfg_data_.at(keys.back());

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
