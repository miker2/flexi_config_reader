#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <iostream>
#include <range/v3/range/conversion.hpp>
#include <string>
#include <tao/pegtl.hpp>
#include <vector>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/exceptions.h"
#include "flexi_cfg/config/grammar.h"
#include "flexi_cfg/config/helpers.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/reader.h"
#include "flexi_cfg/utils.h"

namespace flexi_cfg {

Reader::Reader(config::types::CfgMap cfg, const std::string& parent)
    : cfg_data_(std::move(cfg)), parent_name_(parent) {}

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

  // Special handling for the case where 'name' contains a single key (i.e is not a flat key)
  const auto& data = (struct_like != nullptr) ? struct_like->data : cfg_data_;

  return {keys.back(), data};
}

void Reader::convert(const std::string& value_str, config::types::Type type, float& value) {
  if (type != config::types::Type::kNumber) {
    THROW_EXCEPTION(config::MismatchTypeException, "Expected numeric type, but have '{}' type.",
                    type);
  }
  std::size_t len{0};
  value = std::stof(value_str, &len);
  if (len != value_str.size()) {
    THROW_EXCEPTION(config::MismatchTypeException,
                    "Error while converting '{}' to type float. Processed {} of {} characters",
                    value_str, len, value_str.size());
  }
}

void Reader::convert(const std::string& value_str, config::types::Type type, double& value) {
  if (type != config::types::Type::kNumber) {
    THROW_EXCEPTION(config::MismatchTypeException, "Expected numeric type, but have '{}' type.",
                    type);
  }
  std::size_t len{0};
  value = std::stod(value_str, &len);
  if (len != value_str.size()) {
    THROW_EXCEPTION(config::MismatchTypeException,
                    "Error while converting '{}' to type double. Processed {} of {} characters",
                    value_str, len, value_str.size());
  }
}

void Reader::convert(const std::string& value_str, config::types::Type type, int& value) {
  if (type != config::types::Type::kNumber) {
    THROW_EXCEPTION(config::MismatchTypeException, "Expected numeric type, but have '{}' type.",
                    type);
  }
  std::size_t len{0};
  value = std::stoi(value_str, &len);
  if (len != value_str.size()) {
    THROW_EXCEPTION(config::MismatchTypeException,
                    "Error while converting '{}' to type int. Processed {} of {} characters",
                    value_str, len, value_str.size());
  }
}

void Reader::convert(const std::string& value_str, config::types::Type type, int64_t& value) {
  if (type != config::types::Type::kNumber) {
    THROW_EXCEPTION(config::MismatchTypeException, "Expected numeric type, but have '{}' type.",
                    type);
  }
  std::size_t len{0};
  value = std::stoll(value_str, &len);
  if (len != value_str.size()) {
    THROW_EXCEPTION(config::MismatchTypeException,
                    "Error while converting '{}' to type int64_t. Processed {} of {} characters",
                    value_str, len, value_str.size());
  }
}

void Reader::convert(const std::string& value_str, config::types::Type type, bool& value) {
  if (type != config::types::Type::kBoolean) {
    THROW_EXCEPTION(config::MismatchTypeException, "Expected boolean type, but have '{}' type.",
                    type);
  }
  value = value_str == "true";
}

void Reader::convert(const std::string& value_str, config::types::Type type, std::string& value) {
  if (type != config::types::Type::kString) {
    THROW_EXCEPTION(config::MismatchTypeException, "Expected string type, but have '{}' type.",
                    type);
  }
  value = value_str;
  value.erase(std::remove(std::begin(value), std::end(value), '\"'), std::end(value));
}

void Reader::getValue(const std::string& name, Reader& reader) const {
  // Split the key into parts
  const auto keys = utils::split(name, '.');
  auto cfg_value = config::helpers::getConfigValue(cfg_data_, keys);
  const auto struct_like = dynamic_pointer_cast<config::types::ConfigStructLike>(cfg_value);
  if (struct_like == nullptr) {
    // throw an exception here
    THROW_EXCEPTION(config::MismatchTypeException,
                    "Expected struct type when reading {}, but have '{}' type.",
                    utils::makeName(parent_name_, name), cfg_value->type);
  }

  reader = Reader(struct_like->data, name);
}

}  // namespace flexi_cfg
