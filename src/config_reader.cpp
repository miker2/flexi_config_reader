#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <iostream>
#include <range/v3/range/conversion.hpp>
#include <string>
#include <tao/pegtl.hpp>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/exceptions.h"
#include "flexi_cfg/config/grammar.h"
#include "flexi_cfg/config/helpers.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/reader.h"
#include "flexi_cfg/utils.h"

namespace {

const std::unordered_map<std::type_index, std::string_view> type_names = {
    {{typeid(int)}, "int"},           {{typeid(float)}, "float"},
    {{typeid(double)}, "double"},     {{typeid(uint8_t)}, "uint8_t"},
    {{typeid(uint16_t)}, "uint16_t"}, {{typeid(uint32_t)}, "uint32_t"},
    {{typeid(uint64_t)}, "uint64_t"}, {{typeid(int8_t)}, "int8_t"},
    {{typeid(int16_t)}, "int16_t"},   {{typeid(int32_t)}, "int32_t"},
    {{typeid(int64_t)}, "int64_t"},   {{typeid(bool)}, "bool"},
    {{typeid(std::string)}, "string"}};

/// \brief A helper for converting strings to numeric values
/// \param[in] value_ptr A reference to a ValuePtr object
/// \param[in/out] value The object in which to read the ValuePtr result
/// \param[in] type_str String representation of the value type
/// \param[in] converter The function that will convert from a string to the specified type (e.g.
/// std::stoi, std::stod, etc)
template <typename T>
void numericConversionHelper(const flexi_cfg::config::types::ValuePtr& value_ptr, T& value,
                             std::function<T(const std::string&, std::size_t*)> converter) {
  if (value_ptr->type != flexi_cfg::config::types::Type::kNumber) {
    THROW_EXCEPTION(flexi_cfg::config::MismatchTypeException,
                    "Expected numeric type, but have '{}' type.", value_ptr->type);
  }

  std::size_t len{0};
  value = converter(value_ptr->value, &len);

  if (len != value_ptr->value.size()) {
    const std::type_index type_idx(typeid(T));
    const auto& type_str = type_names.contains(type_idx) ? type_names.at(type_idx) : "type_unknown";
    THROW_EXCEPTION(flexi_cfg::config::MismatchTypeException,
                    "Error while converting '{}' to type {}. Processed {} of {} characters",
                    value_ptr->value, type_str, len, value_ptr->value.size());
  }
}
}  // namespace

namespace flexi_cfg {

Reader::Reader(config::types::CfgMap cfg, std::string parent)
    : cfg_data_(std::move(cfg)), parent_name_(std::move(parent)) {}

void Reader::dump() const { std::cout << cfg_data_; }

auto Reader::exists(const std::string& key) const -> bool {
  try {
    const auto [final_key, data] = getNestedConfig(key);
    return data.find(final_key) != std::end(data);
  } catch (const config::InvalidKeyException&) {
    // The penultimate key exists, but it doesn't contain the last key.
    return false;
  } catch (const config::InvalidTypeException&) {
    // The penultimate key exists, but it isn't a struct, so trying to traverse further fails.
    return false;
  }
}

auto Reader::keys() const -> std::vector<std::string> {
  return cfg_data_ | ranges::views::keys | ranges::to<std::vector<std::string>>;
}

auto Reader::getType(const std::string& key) const -> config::types::Type {
  // Split the key into parts
  const auto keys = utils::split(key, '.');

  const auto cfg_val = config::helpers::getConfigValue(cfg_data_, keys);

  return cfg_val->type;
}

auto Reader::findStructsWithKey(const std::string& key) const -> std::vector<std::string> {
  std::vector<std::string> structs{};

  std::function<void(const std::string&, const config::types::CfgMap&)> contains_key =
      [&key = std::as_const(key), &structs, &contains_key](const std::string& root,
                                                           const config::types::CfgMap& cfg) {
        for (const auto& [k, val] : cfg) {
          if (k == key) {
            // Do something to keep track of this
            logger::trace("Found key: '{}' in '{}'", k, root);
            structs.emplace_back(root);
          }

          if (config::helpers::isStructLike(val)) {
            contains_key(utils::makeName(root, k),
                         dynamic_pointer_cast<config::types::ConfigStructLike>(val)->data);
          }
        }
      };

  contains_key("", cfg_data_);
  return structs;
}

auto Reader::getNestedConfig(const std::string& key) const
    -> std::pair<std::string, const config::types::CfgMap&> {
  // Split the key into parts
  const auto keys = utils::split(key, '.');

  const auto struct_like = config::helpers::getNestedConfig(cfg_data_, keys);

  // Special handling for the case where 'key' contains a single key (i.e is not a flat key)
  const auto& data = (struct_like != nullptr) ? struct_like->data : cfg_data_;

  return {keys.back(), data};
}

void Reader::convert(const config::types::ValuePtr& value_ptr, float& value) {
  numericConversionHelper<float>(value_ptr, value,
                                 [](const auto& str, auto* l) { return std::stof(str, l); });
}

void Reader::convert(const config::types::ValuePtr& value_ptr, double& value) {
  numericConversionHelper<double>(value_ptr, value,
                                  [](const auto& str, auto* l) { return std::stod(str, l); });
}

void Reader::convert(const config::types::ValuePtr& value_ptr, int& value) {
  numericConversionHelper<int>(value_ptr, value,
                               [](const auto& str, auto* l) { return std::stoi(str, l, 0); });
}

void Reader::convert(const config::types::ValuePtr& value_ptr, int64_t& value) {
  numericConversionHelper<int64_t>(value_ptr, value,
                                   [](const auto& str, auto* l) { return std::stoll(str, l, 0); });
}

void Reader::convert(const config::types::ValuePtr& value_ptr, uint64_t& value) {
  numericConversionHelper<uint64_t>(
      value_ptr, value, [](const auto& str, auto* l) { return std::stoull(str, l, 0); });
}

void Reader::convert(const config::types::ValuePtr& value_ptr, bool& value) {
  if (value_ptr->type != config::types::Type::kBoolean) {
    THROW_EXCEPTION(config::MismatchTypeException, "Expected boolean type, but have '{}' type.",
                    value_ptr->type);
  }
  value = value_ptr->value == "true";
}

void Reader::convert(const config::types::ValuePtr& value_ptr, std::string& value) {
  if (value_ptr->type != config::types::Type::kString) {
    THROW_EXCEPTION(config::MismatchTypeException, "Expected string type, but have '{}' type.",
                    value_ptr->type);
  }
  value = value_ptr->value;
  value.erase(std::remove(std::begin(value), std::end(value), '\"'), std::end(value));
}

void Reader::getValue(const std::string& key, Reader& reader) const {
  // Split the key into parts
  const auto keys = utils::split(key, '.');
  auto cfg_value = config::helpers::getConfigValue(cfg_data_, keys);
  const auto struct_like = dynamic_pointer_cast<config::types::ConfigStructLike>(cfg_value);
  if (struct_like == nullptr) {
    // throw an exception here
    THROW_EXCEPTION(config::MismatchTypeException,
                    "Expected struct type when reading {}, but have '{}' type.",
                    utils::makeName(parent_name_, key), cfg_value->type);
  }

  reader = Reader(struct_like->data, key);
}

}  // namespace flexi_cfg
