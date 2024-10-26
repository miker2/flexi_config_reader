#pragma once

#include <fmt/format.h>

#include <memory>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/utils.h"
#include "flexi_cfg/visitor.h"

namespace flexi_cfg::visitor::internal {

template <typename T>
bool tryCast(const std::any& any_val, T& value) {
  try {
    value = std::any_cast<T>(any_val);
    return true;
  } catch (std::bad_any_cast&) {
    return false;
  }
}

template <TypedVisitor Visitor>
void visitStruct(const config::types::CfgMap& cfg, Visitor& visitor);

template <TypedVisitor Visitor>
void visitValue(const std::string& key, std::shared_ptr<config::types::ConfigBase> cfg_val,
                Visitor& visitor) {
  switch (cfg_val->type) {
    case config::types::Type::kString: {
      if constexpr (visitor::StringValueVisitor<Visitor>) {
        auto config = std::dynamic_pointer_cast<config::types::ConfigValue>(cfg_val);
        if (config != nullptr) {
          auto value = config->value;
          // matches Reader::convert(..)
          value.erase(std::remove(std::begin(value), std::end(value), '\"'), std::end(value));
          visitor.onValue(value);
          break;
        }
      }
    }

    case config::types::Type::kNumber: {
      auto config = std::dynamic_pointer_cast<config::types::ConfigValue>(cfg_val);
      if (config != nullptr) {
        if constexpr (visitor::IntValueVisitor<Visitor>) {
          if (int8_t i_val; tryCast(config->value_any, i_val)) {
            visitor.onValue(static_cast<int64_t>(i_val));
            break;
          }
          if (uint8_t ui_val; tryCast(config->value_any, ui_val)) {
            visitor.onValue(static_cast<uint64_t>(ui_val));
            break;
          }
          if (int16_t i_val; tryCast(config->value_any, i_val)) {
            visitor.onValue(static_cast<int64_t>(i_val));
            break;
          }
          if (uint16_t ui_val; tryCast(config->value_any, ui_val)) {
            visitor.onValue(static_cast<uint64_t>(ui_val));
            break;
          }
          if (int32_t i_val; tryCast(config->value_any, i_val)) {
            visitor.onValue(static_cast<int64_t>(i_val));
            break;
          }
          if (uint32_t ui_val; tryCast(config->value_any, ui_val)) {
            visitor.onValue(static_cast<uint64_t>(ui_val));
            break;
          }
          if (int64_t i_val; tryCast(config->value_any, i_val)) {
            visitor.onValue(i_val);
            break;
          }
          if (uint64_t ui_val; tryCast(config->value_any, ui_val)) {
            visitor.onValue(ui_val);
            break;
          }
          if (long long i_val; tryCast(config->value_any, i_val)) {
            visitor.onValue(static_cast<int64_t>(i_val));
            break;
          }
          if (unsigned long long ui_val; tryCast(config->value_any, ui_val)) {
            visitor.onValue(static_cast<uint64_t>(ui_val));
            break;
          }
        }
        if constexpr (visitor::FloatValueVisitor<Visitor>) {
          if (float f_val; tryCast(config->value_any, f_val)) {
            visitor.onValue(f_val);
            break;
          }
          if (double d_val; tryCast(config->value_any, d_val)) {
            visitor.onValue(d_val);
            break;
          }
          if (long double d_val; tryCast(config->value_any, d_val)) {
            visitor.onValue(static_cast<double>(d_val));
            break;
          }
        }
      }
    }

    case config::types::Type::kBoolean: {
      if constexpr (visitor::BoolValueVisitor<Visitor>) {
        auto config = std::dynamic_pointer_cast<config::types::ConfigValue>(cfg_val);
        if (bool b_val; config != nullptr && tryCast(config->value_any, b_val)) {
          visitor.onValue(b_val);
          break;
        }
      }
    }

    case config::types::Type::kStruct:
    case config::types::Type::kStructInProto: {
      if constexpr (visitor::StructVisitor<Visitor>) {
        auto config = std::dynamic_pointer_cast<config::types::ConfigStructLike>(cfg_val);
        if (config != nullptr) {
          visitStruct(config->data, visitor);
          break;
        }
      }
    }

    case config::types::Type::kList: {
      if constexpr (visitor::ListVisitor<Visitor>) {
        auto config = std::dynamic_pointer_cast<config::types::ConfigList>(cfg_val);
        if (config != nullptr) {
          visitor.beginList();
          for (auto list_cfg_val : config->data) {
            visitValue(key, list_cfg_val, visitor);
          }
          visitor.endList();
          break;
        }
      }
    }

    default:
      logger::warn("Visitor, unhandled key: {} -- Type: {} ", key,
                   magic_enum::enum_name(cfg_val->type));
      break;
  }
}

template <TypedVisitor Visitor>
void visitStruct(const config::types::CfgMap& cfg, Visitor& visitor) {
  if constexpr (visitor::StructVisitor<Visitor>) {
    visitor.beginStruct();
  }
  for (const auto& key : cfg | ranges::views::keys | ranges::to<std::vector<std::string>>) {
    auto cfg_val = cfg.at(key);
    if constexpr (visitor::KeyVisitor<Visitor>) {
      visitor.onKey(key);
    }
    visitValue(key, cfg_val, visitor);
  }
  if constexpr (visitor::StructVisitor<Visitor>) {
    visitor.endStruct();
  }
}

}  // namespace flexi_cfg::visitor::internal
