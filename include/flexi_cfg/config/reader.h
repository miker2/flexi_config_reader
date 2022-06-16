#pragma once

#include <fmt/format.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/exceptions.h"
#include "flexi_cfg/config/grammar.h"
#include "flexi_cfg/config/helpers.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/utils.h"

class ConfigReader {
 public:
  ConfigReader() = default;
  ~ConfigReader() = default;

  ConfigReader(const ConfigReader&) = delete;
  auto operator=(const ConfigReader&) -> ConfigReader& = delete;
  ConfigReader(ConfigReader&&) = default;
  auto operator=(ConfigReader&&) -> ConfigReader& = delete;

  auto parse(const std::filesystem::path& cfg_filename) -> bool;

  auto parse(std::string_view cfg_string, std::string_view source = "unknown") -> bool;

  void dump() const;

  template <typename T>
  auto getValue(const std::string& name) const -> T;

  template <typename T>
  void getValue(const std::string& name, T& value) const;

  template <typename T>
  void getValue(const std::string& name, std::vector<T>& value) const;

  template <typename T, size_t N>
  void getValue(const std::string& name, std::array<T, N>& value) const;

 private:
  static void convert(const std::string& value_str, float& value);
  static void convert(const std::string& value_str, double& value);
  static void convert(const std::string& value_str, int& value);
  static void convert(const std::string& value_str, int64_t& value);
  static void convert(const std::string& value_str, bool& value);
  static void convert(const std::string& value_str, std::string& value);

  void resolveConfig();

  auto flattenAndFindProtos(const config::types::CfgMap& in, const std::string& base_name,
                            config::types::CfgMap flattened = {}) -> config::types::CfgMap;

  /// \brief Remove the protos from merged dictionary
  /// \param[in/out] cfg_map - The top level (resolved) config map
  void stripProtos(config::types::CfgMap& cfg_map) const;

  /// \brief Walk through CfgMap and find all references. Convert them to structs
  /// \param[in/out] cfg_map
  /// \param[in] base_name - starting point for resolving references
  /// \param[in] ref_vars - map of all reference variables available in the current context
  /// \param[in] refd_protos - vector of all protos already referenced. Used to track cycles.
  void resolveReferences(config::types::CfgMap& cfg_map, const std::string& base_name,
                         const config::types::RefMap& ref_vars = {},
                         const std::vector<std::string>& refd_protos = {}) const;

  config::ActionData out_;
  config::types::ProtoMap protos_{};

  // All of the config data!
  config::types::CfgMap cfg_data_;
};

template <typename T>
void ConfigReader::getValue(const std::string& name, T& value) const {
  // Split the key into parts
  const auto keys = utils::split(name, '.');

  const auto struct_like = config::helpers::getNestedConfig(cfg_data_, keys);

  // Special handling for the case where 'name' contains a single key (i.e is not a flat key)
  const auto cfg_val =
      (struct_like != nullptr) ? struct_like->data.at(keys.back()) : cfg_data_.at(keys.back());

  const auto value_str = dynamic_pointer_cast<config::types::ConfigValue>(cfg_val)->value;
  convert(value_str, value);
  logger::debug(" -- Type is {}", typeid(T).name());
}

template <typename T>
void ConfigReader::getValue(const std::string& name, std::vector<T>& value) const {
  // Split the key into parts
  const auto keys = utils::split(name, '.');

  const auto struct_like = config::helpers::getNestedConfig(cfg_data_, keys);

  // Special handling for the case where 'name' contains a single key (i.e is not a flat key)
  const auto cfg_val =
      (struct_like != nullptr) ? struct_like->data.at(keys.back()) : cfg_data_.at(keys.back());

  // Ensure this is a list if the user is asking for a list.
  if (cfg_val->type != config::types::Type::kList) {
    throw config::InvalidTypeException(
        fmt::format("Expected '{}' to contain a list, but is of type {}", name, cfg_val->type));
  }

  const auto& list = dynamic_pointer_cast<config::types::ConfigList>(cfg_val)->data;
  for (const auto& e : list) {
    const auto value_str = dynamic_pointer_cast<config::types::ConfigValue>(e)->value;
    T v{};
    convert(value_str, v);
    value.emplace_back(v);
  }
}

template <typename T, size_t N>
void ConfigReader::getValue(const std::string& name, std::array<T, N>& value) const {
  // Split the key into parts
  const auto keys = utils::split(name, '.');

  const auto struct_like = config::helpers::getNestedConfig(cfg_data_, keys);

  // Special handling for the case where 'name' contains a single key (i.e is not a flat key)
  const auto cfg_val =
      (struct_like != nullptr) ? struct_like->data.at(keys.back()) : cfg_data_.at(keys.back());

  // Ensure this is a list if the user is asking for a list.
  if (cfg_val->type != config::types::Type::kList) {
    throw config::InvalidTypeException(
        fmt::format("Expected '{}' to contain a list, but is of type {}", name, cfg_val->type));
  }

  const auto& list = dynamic_pointer_cast<config::types::ConfigList>(cfg_val)->data;
  if (list.size() != N) {
    throw std::runtime_error(
        fmt::format("Expected {} entries in '{}', but found {}!", N, cfg_val, list.size()));
  }
  for (size_t i = 0; i < N; ++i) {
    const auto value_str = dynamic_pointer_cast<config::types::ConfigValue>(list[i])->value;
    convert(value_str, value[i]);
  }
}

template <typename T>
auto ConfigReader::getValue(const std::string& name) const -> T {
  T value{};
  getValue(name, value);
  return value;
}
