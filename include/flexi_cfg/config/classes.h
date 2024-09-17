#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <any>
#include <iostream>
#include <magic_enum.hpp>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "flexi_cfg/utils.h"
#include "flexi_cfg/logger.h"

#define DEBUG_CLASSES 0
#define PRINT_SRC 0  // NOLINT(cppcoreguidelines-macro-usage)

namespace flexi_cfg::config::types {
constexpr std::size_t tw{4};  // The width of the indentation

class ConfigBase;
using BasePtr = std::shared_ptr<ConfigBase>;

using CfgMap = std::map<std::string, BasePtr>;
using RefMap = std::map<std::string, BasePtr>;
class ConfigProto;
using ProtoMap = std::map<std::string, std::shared_ptr<ConfigProto>>;

class ConfigValue;
using ValuePtr = std::shared_ptr<ConfigValue>;

enum class Type {
  kValue,
  kString,
  kNumber,
  kBoolean,
  kList,
  kExpression,
  kValueLookup,
  kVar,
  kStruct,
  kStructInProto,
  kProto,
  kReference,
  kUnknown
};

inline auto operator<<(std::ostream& os, const Type& type) -> std::ostream& {
  return os << magic_enum::enum_name<config::types::Type>(type);
}

// This is the base-class from which all config nodes shall derive
class ConfigBase {
 public:
  virtual ~ConfigBase() noexcept = default;
  auto operator=(const ConfigBase&) -> ConfigBase& = delete;
  auto operator=(ConfigBase&&) -> ConfigBase& = delete;

  virtual void stream(std::ostream&) const = 0;

  [[nodiscard]] virtual auto clone() const -> BasePtr = 0;

  [[nodiscard]] auto loc() const -> std::string { return fmt::format("{}:{}", source, line); }

  const Type type;

  std::size_t line{0};
  std::string source{};

 protected:
  explicit ConfigBase(const Type in_type) : type{in_type} {}

  ConfigBase(const ConfigBase&) = default;
  ConfigBase(ConfigBase&&) = default;
};

// Use CRTP to add "clone" method to each class.
// A slight modification of the concept found here:
//  https://herbsutter.com/2019/10/03/gotw-ish-solution-the-clonable-pattern/
// To bad meta-classes and reflection aren't supported. The result from that post is very clean.
template <typename Base, typename Derived>
class ConfigBaseClonable : public Base {
 public:
  using Base::Base;

  [[nodiscard]] auto clone() const -> BasePtr override {
    // This sort of feels like a dirty hack, but appears to work.
    // See: https://stackoverflow.com/a/25069711
    struct make_shared_enabler : public Derived {};
    return std::make_shared<make_shared_enabler>(static_cast<const make_shared_enabler&>(*this));
  }
};

inline auto operator<<(std::ostream& os, const ConfigBase& cfg) -> std::ostream& {
  cfg.stream(os);
  return os;
}

template <typename T>
inline auto operator<<(std::ostream& os, const std::shared_ptr<T>& cfg) -> std::ostream& {
  return (cfg ? (os << *cfg) : (os << "NULL"));
}

class ConfigStructLike;
template <typename Key, typename Value>
inline auto operator<<(std::ostream& os, const std::map<Key, Value>& data) -> std::ostream& {
  for (const auto& kv : data) {
    if (dynamic_pointer_cast<ConfigStructLike>(kv.second)) {
      os << kv.second << "\n";
    } else {
      os << kv.first << " = " << kv.second
#if PRINT_SRC
         << "  # " << kv.second->loc()
#endif
         << "\n";
    }
  }
  return os;
}

// See here for a potentially better solution:
//    https://raw.githubusercontent.com/louisdx/cxx-prettyprint/master/prettyprint.hpp
template <typename Key, typename Value>
inline void pprint(std::ostream& os, const std::map<Key, std::shared_ptr<Value>>& data,
                   std::size_t depth) {
  const auto ws = std::string(depth * tw, ' ');
  for (const auto& kv : data) {
    if (dynamic_pointer_cast<ConfigStructLike>(kv.second)) {
      // Don't add extra whitespace, as this is handled entirely by the StructLike objects
      os << kv.second << "\n";
    } else {
      os << ws << kv.first << " = " << kv.second
#if PRINT_SRC
         << "  # " << kv.second->loc()
#endif
         << "\n";
    }
  }
}

template <typename Key, typename Value>
inline void pprint(std::ostream& os, const std::map<Key, Value>& data, std::size_t depth) {
  const auto ws = std::string(depth * tw, ' ');
  for (const auto& kv : data) {
    os << ws << kv.first << " = " << kv.second
#if PRINT_SRC
       << "  # " << kv.second->loc()
#endif
       << "\n";
  }
}

class ConfigValue : public ConfigBaseClonable<ConfigBase, ConfigValue> {
 public:
  explicit ConfigValue(std::string value_in, Type type, std::any val = {})
      : ConfigBaseClonable(type), value{std::move(value_in)}, value_any{std::move(val)} {};

  void stream(std::ostream& os) const override { os << value; }

  const std::string value{};

  const std::any value_any{};

  ~ConfigValue() noexcept override = default;
  auto operator=(const ConfigValue&) -> ConfigValue& = delete;
  auto operator=(ConfigValue&&) -> ConfigValue& = delete;

 protected:
  ConfigValue(const ConfigValue&) = default;
  ConfigValue(ConfigValue&&) = default;
};

class ConfigList : public ConfigBaseClonable<ConfigValue, ConfigList> {
 public:
  ConfigList(std::string value_in = "") : ConfigBaseClonable(std::move(value_in), Type::kList){};

  void stream(std::ostream& os) const override {
    os << "[";
    for (size_t i = 0; i < data.size() - 1; ++i) {
      os << data[i] << ", ";
    }
    os << data.back() << "]";
  }

  std::vector<std::shared_ptr<ConfigBase>> data;

  Type list_element_type{Type::kUnknown};

  ~ConfigList() noexcept override = default;
  auto operator=(const ConfigList&) -> ConfigList& = delete;
  auto operator=(ConfigList&&) -> ConfigList& = delete;

 protected:
  ConfigList(const ConfigList&) = default;
  ConfigList(ConfigList&&) = default;
};

class ConfigValueLookup : public ConfigBaseClonable<ConfigBase, ConfigValueLookup> {
 public:
  explicit ConfigValueLookup(const std::string& var_ref)
      : ConfigBaseClonable(Type::kValueLookup), keys{utils::split(var_ref, '.')} {};

  void stream(std::ostream& os) const override { os << "$(" << var() << ")"; }

  const std::vector<std::string> keys{};

  [[nodiscard]] auto var() const -> std::string { return utils::join(keys, "."); }

  ~ConfigValueLookup() noexcept override = default;
  auto operator=(const ConfigValueLookup&) -> ConfigValueLookup& = delete;
  auto operator=(ConfigValueLookup&&) -> ConfigValueLookup& = delete;

 protected:
  ConfigValueLookup(const ConfigValueLookup&) = default;
  ConfigValueLookup(ConfigValueLookup&&) = default;
};

// ConfigExpression is a special "value" type. We want the same interface as ConfigValue because it
// facilitates certain operations, but we also want to track the enclosed "ConfigValueLookup"
// strings
class ConfigExpression : public ConfigBaseClonable<ConfigValue, ConfigExpression> {
 public:
  explicit ConfigExpression(std::string expression_in, CfgMap val_lookups)
      : ConfigBaseClonable(std::move(expression_in), Type::kExpression),
        value_lookups{std::move(val_lookups)} {};

  CfgMap value_lookups{};

  ~ConfigExpression() noexcept override = default;
  auto operator=(const ConfigExpression&) -> ConfigExpression& = delete;
  auto operator=(ConfigExpression&&) -> ConfigExpression& = delete;

 protected:
  ConfigExpression(const ConfigExpression&) = default;

  ConfigExpression(ConfigExpression&&) = default;
};

class ConfigVar : public ConfigBaseClonable<ConfigBase, ConfigVar> {
 public:
  explicit ConfigVar(std::string name) : ConfigBaseClonable(Type::kVar), name{std::move(name)} {};

  void stream(std::ostream& os) const override { os << name; }

  const std::string name{};

  ~ConfigVar() noexcept override = default;
  auto operator=(const ConfigVar&) -> ConfigVar& = delete;
  auto operator=(ConfigVar&&) -> ConfigVar& = delete;

 protected:
  ConfigVar(const ConfigVar&) = default;
  ConfigVar(ConfigVar&&) = default;
};

inline auto operator<<(std::ostream& os, const ConfigVar& cfg) -> std::ostream& {
  cfg.stream(os);
  return os;
}

class ConfigStructLike : public ConfigBaseClonable<ConfigBase, ConfigStructLike> {
 public:
  ConfigStructLike(Type in_type, std::string name, std::size_t depth)
      : ConfigBaseClonable(in_type), name{std::move(name)}, depth{depth} {};

  void stream(std::ostream& os) const override {
    os << "struct-like " << name << "\n";
    os << data;
  }

  const std::string name{};

  std::size_t depth{};

  CfgMap data;

  auto operator[](const std::string& key) const -> const BasePtr& { return data.at(key); }

  auto operator[](const std::string& key) -> BasePtr& { return data[key]; }

  ~ConfigStructLike() noexcept override = default;
  auto operator=(const ConfigStructLike&) -> ConfigStructLike& = delete;
  auto operator=(ConfigStructLike&&) -> ConfigStructLike& = delete;

 protected:
  ConfigStructLike(const ConfigStructLike&) = default;
  ConfigStructLike(ConfigStructLike&&) = default;
};

class ConfigStruct : public ConfigBaseClonable<ConfigStructLike, ConfigStruct> {
 public:
  ConfigStruct(std::string name, std::size_t depth, Type type = Type::kStruct)
      : ConfigBaseClonable(type, std::move(name), depth){};

  void stream(std::ostream& os) const override {
    const auto ws = std::string(depth * tw, ' ');
    os << ws << "struct " << name << " {\n";
#if DEBUG_CLASSES
    os << ws << "-- " << data.size() << " k/v pairs\n";
#endif
    pprint(os, data, depth + 1);
    // os << data;
    os << ws << "}";
  }

  [[nodiscard]] auto clone() const -> BasePtr final {
    // This sort of feels like a dirty hack, but appears to work.
    // See: https://stackoverflow.com/a/25069711
    struct make_shared_enabler : public ConfigStruct {};
    auto cloned =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        std::make_shared<make_shared_enabler>(static_cast<const make_shared_enabler&>(*this));
    for (const auto& kv : data) {
      (*cloned)[kv.first] = kv.second->clone();
    }
    return cloned;
  }

  ~ConfigStruct() noexcept override = default;
  auto operator=(const ConfigStruct&) -> ConfigStruct& = delete;
  auto operator=(ConfigStruct&&) -> ConfigStruct& = delete;

 protected:
  ConfigStruct(const ConfigStruct&) = default;
  ConfigStruct(ConfigStruct&&) = default;
};

class ConfigProto : public ConfigBaseClonable<ConfigStructLike, ConfigProto> {
 public:
  ConfigProto(std::string name, std::size_t depth)
      : ConfigBaseClonable(Type::kProto, std::move(name), depth) {}

  void stream(std::ostream& os) const override {
    const auto ws = std::string(depth * tw, ' ');
    os << ws << "proto " << name << " {\n";
#if DEBUG_CLASSES
    os << ws << "-- " << data.size() << " k/v pairs\n";
#endif
    pprint(os, data, depth + 1);
    os << ws << "}";
  }

  [[nodiscard]] auto clone() const -> BasePtr final {
    // This sort of feels like a dirty hack, but appears to work.
    // See: https://stackoverflow.com/a/25069711
    struct make_shared_enabler : public ConfigProto {};
    auto cloned =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        std::make_shared<make_shared_enabler>(static_cast<const make_shared_enabler&>(*this));
    for (const auto& kv : data) {
      (*cloned)[kv.first] = kv.second->clone();
    }
    return cloned;
  }

  ~ConfigProto() noexcept override = default;
  auto operator=(const ConfigProto&) -> ConfigProto& = delete;
  auto operator=(ConfigProto&&) -> ConfigProto& = delete;

 protected:
  ConfigProto(const ConfigProto&) = default;
  ConfigProto(ConfigProto&&) = default;
};

class ConfigReference : public ConfigBaseClonable<ConfigStructLike, ConfigReference> {
 public:
  ConfigReference(const std::string& name, std::string proto_name, std::size_t depth)
      : ConfigBaseClonable(Type::kReference, name, depth), proto{std::move(proto_name)} {
    // Create the required key to easily reference the parent name.
    ref_vars["$PARENT_NAME"] = std::make_shared<ConfigValue>(name, Type::kString);
  }

  void stream(std::ostream& os) const override {
    const auto ws = std::string(depth * tw, ' ');
    os << ws << "reference " << proto << " as " << name << " {\n";
#if DEBUG_CLASSES
    os << ws << "-- " << ref_vars.size() << " ref vars\n";
    os << ws << "-- " << data.size() << " k/v pairs\n";
#endif
    pprint(os, ref_vars, depth + 1);
    pprint(os, data, depth + 1);
    os << ws << "}";
  }

  const std::string proto{};

  RefMap ref_vars;

  ~ConfigReference() noexcept override = default;
  auto operator=(const ConfigReference&) -> ConfigReference& = delete;
  auto operator=(ConfigReference&&) -> ConfigReference& = delete;

 protected:
  ConfigReference(const ConfigReference&) = default;
  ConfigReference(ConfigReference&&) = default;
};

};  // namespace flexi_cfg::config::types

template <>
struct fmt::formatter<flexi_cfg::config::types::Type> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>
  FMT_CONSTEXPR auto format(const flexi_cfg::config::types::Type& type, format_context& ctx) const {
    const auto type_s = magic_enum::enum_name<flexi_cfg::config::types::Type>(type);
    return formatter<std::string_view>::format(type_s, ctx);
  }
};

// Formatter for all types that inherit from `config::types::ConfigBase`.
template <typename T>
struct fmt::formatter<
    T, std::enable_if_t<
           std::is_convertible_v<T, std::shared_ptr<flexi_cfg::config::types::ConfigBase>>, char>>
    : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>
  auto format(const std::shared_ptr<flexi_cfg::config::types::ConfigBase>& cfg,
              format_context& ctx) const {
    std::stringstream ss;
    if (cfg != nullptr) {
      cfg->stream(ss);
    } else {
      ss << "NULL";
    }
    return formatter<std::string_view>::format(ss.str(), ctx);
  }
};
