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
#include "flexi_cfg/config/helpers.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/utils.h"

namespace flexi_cfg {

class Reader {
 public:
  explicit Reader(config::types::CfgMap cfg, std::string parent = "");
  Reader() = default;
  ~Reader() = default;

  Reader(const Reader&) = default;
  auto operator=(const Reader&) -> Reader& = default;
  Reader(Reader&&) = default;
  auto operator=(Reader&&) -> Reader& = default;

  /// \brief Prints the full config to the console
  void dump() const;

  /// \brief Checks if an entry with the provided key exists
  /// \param[in] key The name of the key of interest
  /// \return True if the key exists
  [[nodiscard]] auto exists(const std::string& key) const -> bool;

  /// \brief Provides the keys for the first level of the config structure
  /// \return A vector keys
  [[nodiscard]] auto keys() const -> std::vector<std::string>;

  /// \brief Accessor to the value of the given key (if it exists)
  /// \param[in] key The name of the key of interest
  /// \return The value of the key
  template <typename T>
  auto getValue(const std::string& key) const -> T;

  /// \brief Accessor to the value of the given key (if it exists)
  /// \param[in] key The name of the key of interest
  /// \param[out] value The value of the key
  template <typename T>
  void getValue(const std::string& key, T& value) const;

  template <typename T>
  void getValue(const std::string& key, std::vector<T>& value) const;

  template <typename T, size_t N>
  void getValue(const std::string& key, std::array<T, N>& value) const;

  /// \brief Provides a list of all structs containing the specified key
  /// \param[in] key The name of the key to search for recursively within all structs
  /// \return A vector of keys for all structs containing 'key'
  [[nodiscard]] auto findStructsWithKey(const std::string& key) const -> std::vector<std::string>;

 protected:
  [[nodiscard]] auto getNestedConfig(const std::string& key) const
      -> std::pair<std::string, const config::types::CfgMap&>;

  /// \note: This method is here in order to enable the python bindings to more easily parse list types
  [[nodiscard]] auto getCfgMap() const -> const config::types::CfgMap& { return cfg_data_; }

  static void convert(const config::types::ValuePtr& value_ptr, float& value);
  static void convert(const config::types::ValuePtr& value_ptr, double& value);
  static void convert(const config::types::ValuePtr& value_ptr, int& value);
  static void convert(const config::types::ValuePtr& value_ptr, int64_t& value);
  static void convert(const config::types::ValuePtr& value_ptr, uint64_t& value);
  static void convert(const config::types::ValuePtr& value_ptr, bool& value);
  static void convert(const config::types::ValuePtr& value_ptr, std::string& value);

 private:
  template <typename T>
  static void convert(const config::types::ValuePtr& value_ptr, std::vector<T>& value);
  template <typename T, size_t N>
  static void convert(const config::types::ValuePtr& value_ptr, std::array<T, N>& value);

  void getValue(const std::string& key, Reader& reader) const;

  // All of the config data!
  config::types::CfgMap cfg_data_;
  // Store the name of the parent struct for debugging/printing
  std::string parent_name_;
};

template <typename T>
auto Reader::getValue(const std::string& key) const -> T {
  T value{};
  getValue(key, value);
  return value;
}

template <typename T>
void Reader::getValue(const std::string& key, T& value) const {
  // Split the key into parts
  const auto keys = utils::split(key, '.');

  const auto cfg_val = config::helpers::getConfigValue(cfg_data_, keys);

  try {
    const auto value_ptr = dynamic_pointer_cast<config::types::ConfigValue>(cfg_val);
    convert(value_ptr, value);
    logger::debug(" -- Type is {}", typeid(T).name());
  } catch (const std::runtime_error& e) {
    // Catch and re-throw with extra information to aid in debugging.
    THROW_EXCEPTION(std::runtime_error, "While reading '{}': {}",
                    utils::makeName(parent_name_, key), e.what());
  }
}

template <typename T>
void Reader::convert(const std::shared_ptr<config::types::ConfigValue>& value_ptr,
                     std::vector<T>& value) {
  logger::debug("In Reader::convert for T = '{}'", typeid(T).name());

  assert(value_ptr != nullptr && "value_ptr is a nullptr");

  const auto list_ptr = dynamic_pointer_cast<config::types::ConfigList>(value_ptr);
  if (list_ptr == nullptr) {
    THROW_EXCEPTION(std::runtime_error, "Expected '{}' type but got '{}' type.",
                    config::types::Type::kList, value_ptr->type);
  }
  logger::debug("List values: '[{}]'", fmt::join(list_ptr->data, ", "));

  for (const auto& e : list_ptr->data) {
    const auto list_value_ptr = dynamic_pointer_cast<config::types::ConfigValue>(e);
    T v{};
    convert(list_value_ptr, v);
    value.emplace_back(v);
  }
}

template <typename T, size_t N>
void Reader::convert(const std::shared_ptr<config::types::ConfigValue>& value_ptr,
                     std::array<T, N>& value) {
  logger::debug("In Reader::convert for T = '{}'", typeid(T).name());
  assert(value_ptr != nullptr && "value_ptr is a nullptr");

  const auto list_ptr = dynamic_pointer_cast<config::types::ConfigList>(value_ptr);
  if (list_ptr == nullptr) {
    THROW_EXCEPTION(std::runtime_error, "Expected '{}' type but got '{}' type.",
                    config::types::Type::kList, value_ptr->type);
  }
  logger::debug("List values: '[{}]'", fmt::join(list_ptr->data, ", "));

  if (list_ptr->data.size() != N) {
    THROW_EXCEPTION(std::runtime_error, "Expected {} entries in '{}', but found {}!", N, value_ptr,
                    list_ptr->data.size());
  }

  for (size_t i = 0; i < N; ++i) {
    const auto list_value_ptr = dynamic_pointer_cast<config::types::ConfigValue>(list_ptr->data[i]);
    convert(list_value_ptr, value[i]);
  }
}

template <typename T>
void Reader::getValue(const std::string& key, std::vector<T>& value) const {
  // Split the key into parts
  const auto keys = utils::split(key, '.');

  const auto cfg_val = config::helpers::getConfigValue(cfg_data_, keys);

  // Ensure this is a list if the user is asking for a list.
  if (cfg_val->type != config::types::Type::kList) {
    THROW_EXCEPTION(config::InvalidTypeException,
                    "Expected '{}' to contain a list, but is of type {}",
                    utils::makeName(parent_name_, key), cfg_val->type);
  }
  logger::debug("Reading vector of type: {}", typeid(T).name());

  try {
    const auto& value_ptr = dynamic_pointer_cast<config::types::ConfigValue>(cfg_val);
    convert(value_ptr, value);
  } catch (const std::runtime_error& e) {
    // Catch and re-throw with extra information to aid in debugging.
    THROW_EXCEPTION(std::runtime_error, "While reading '{}': {}",
                    utils::makeName(parent_name_, key), e.what());
  }
}

template <typename T, size_t N>
void Reader::getValue(const std::string& key, std::array<T, N>& value) const {
  // Split the key into parts
  const auto keys = utils::split(key, '.');

  const auto cfg_val = config::helpers::getConfigValue(cfg_data_, keys);

  // Ensure this is a list if the user is asking for a list.
  if (cfg_val->type != config::types::Type::kList) {
    THROW_EXCEPTION(config::InvalidTypeException,
                    "Expected '{}' to contain a list, but is of type {}",
                    utils::makeName(parent_name_, key), cfg_val->type);
  }
  logger::debug("Reading array of type: {}", typeid(T).name());

  try {
    const auto& value_ptr = dynamic_pointer_cast<config::types::ConfigValue>(cfg_val);
    convert(value_ptr, value);
  } catch (const std::runtime_error& e) {
    // Catch and re-throw with extra information to aid in debugging.
    THROW_EXCEPTION(std::runtime_error, "While reading '{}': {}",
                    utils::makeName(parent_name_, key), e.what());
  }
}

}  // namespace flexi_cfg
