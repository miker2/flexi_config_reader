#pragma once

#include <fmt/format.h>

#include <any>
#include <iostream>
#include <magic_enum.hpp>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "utils.h"

#define DEBUG_CLASSES 0
#define PRINT_SRC 0

// Macros, ick! But... These can be used in place of the CRTP below (maybe easier to manage?)
#define CLONABLE_BASE(Base) virtual auto clone() const->std::shared_ptr<Base> = 0;

#define CLONABLE(Base)                                                       \
  auto clone() const->std::shared_ptr<Base> override {                       \
    using Self = std::remove_cv_t<std::remove_reference_t<decltype(*this)>>; \
    return std::shared_ptr<Self>(new Self(*this));                           \
  }

namespace {
constexpr std::size_t tw{4};  // The width of the indentation
}

namespace config::types {

class ConfigBase;
class ConfigVar;
using CfgMap = std::map<std::string, std::shared_ptr<ConfigBase>>;
using RefMap = std::map<std::string, std::shared_ptr<ConfigBase>>;

enum class Type {
  kValue,
  kString,
  kNumber,
  kValueLookup,
  kVar,
  kStruct,
  kProto,
  kReference,
  kUnknown
};

// This is the base-class from which all config nodes shall derive
class ConfigBase {
 protected:
  explicit ConfigBase(const Type in_type) : type{in_type} {}

  ConfigBase(const ConfigBase&) = default;

  ConfigBase& operator=(const ConfigBase&) = delete;

 public:
  virtual ~ConfigBase() noexcept = default;

  virtual void stream(std::ostream&) const = 0;

  virtual auto clone() const -> std::shared_ptr<ConfigBase> = 0;

  auto loc() const -> std::string { return fmt::format("{}:{}", source, line); }

  const Type type;

  std::size_t line{0};
  std::string source{};
};

// Use CRTP to add "clone" method to each class.
// A slight modification of the concept found here:
//  https://herbsutter.com/2019/10/03/gotw-ish-solution-the-clonable-pattern/
// To bad meta-classes and reflection aren't supported. The result from that post is very clean.
template <typename Base, typename Derived>
class ConfigBaseClonable : public Base {
 public:
  using Base::Base;

  virtual auto clone() const -> std::shared_ptr<ConfigBase> override {
    // This sort of feels like a dirty hack, but appears to work.
    // See: https://stackoverflow.com/a/25069711
    struct make_shared_enabler : public Derived {};
    return std::make_shared<make_shared_enabler>(static_cast<const make_shared_enabler&>(*this));
  }
};

inline std::ostream& operator<<(std::ostream& os, const ConfigBase& cfg) {
  cfg.stream(os);
  return os;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<T>& cfg) {
  return (cfg ? (os << *cfg) : (os << "NULL"));
}

class ConfigStructLike;
template <typename Key, typename Value>
inline std::ostream& operator<<(std::ostream& os, const std::map<Key, Value>& data) {
  for (const auto& kv : data) {
    if (dynamic_pointer_cast<ConfigStructLike>(kv.second)) {
      os << "\n" << kv.second << "\n\n";
    } else {
      os << kv.first << " = " << kv.second << "\n";
    }
  }
  return os;
}

template <typename Key, typename Value>
inline void pprint(std::ostream& os, const std::map<Key, std::shared_ptr<Value>>& data,
                   std::size_t depth) {
  const auto ws = std::string(depth * tw, ' ');
  for (const auto& kv : data) {
    if (dynamic_pointer_cast<ConfigStructLike>(kv.second)) {
      // Don't add extra whitespace, as this is handled entirely by the StructLike objects
      os << "\n" << kv.second << "\n\n";
    } else {
      os << ws << kv.first << " = " << kv.second
#if PRINT_SRC
         << "  # " << kv.second->source << ":" << kv.second->line
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
       << "  # " << kv.second->source << ":" << kv.second->line
#endif
       << "\n";
  }
}

class ConfigValue : public ConfigBaseClonable<ConfigBase, ConfigValue> {
 public:
  explicit ConfigValue(std::string value_in, Type type = Type::kValue, std::any val = {})
      : ConfigBaseClonable(type), value{value_in}, value_any{std::move(val)} {};

  void stream(std::ostream& os) const override { os << value; }

  const std::string value{};

  const std::any value_any{};

  ~ConfigValue() noexcept override = default;

 protected:
  ConfigValue(const ConfigValue&) = default;
  ConfigValue& operator=(const ConfigValue&) = delete;
};

class ConfigValueLookup : public ConfigBaseClonable<ConfigBase, ConfigValueLookup> {
 public:
  ConfigValueLookup(const std::string& var_ref)
      : ConfigBaseClonable(Type::kValueLookup), keys{utils::split(var_ref, '.')} {};

  void stream(std::ostream& os) const override { os << "$(" << var() << ")"; }

  const std::vector<std::string> keys{};

  auto var() const -> std::string { return utils::join(keys, "."); }

  ~ConfigValueLookup() noexcept override = default;

 protected:
  ConfigValueLookup(const ConfigValueLookup&) = default;
  ConfigValueLookup& operator=(const ConfigValueLookup&) = delete;
};

class ConfigVar : public ConfigBaseClonable<ConfigBase, ConfigVar> {
 public:
  ConfigVar(std::string name) : ConfigBaseClonable(Type::kVar), name{std::move(name)} {};

  void stream(std::ostream& os) const override { os << name; }

  const std::string name{};

  ~ConfigVar() noexcept override = default;

 protected:
  ConfigVar(const ConfigVar&) = default;
  ConfigVar& operator=(const ConfigVar&) = delete;
};

inline std::ostream& operator<<(std::ostream& os, const ConfigVar& cfg) {
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

  const std::size_t depth{};

  CfgMap data;

  ~ConfigStructLike() noexcept override = default;

 protected:
  ConfigStructLike(const ConfigStructLike&) = default;
  ConfigStructLike& operator=(const ConfigStructLike&) = delete;
};

class ConfigStruct : public ConfigBaseClonable<ConfigStructLike, ConfigStruct> {
 public:
  ConfigStruct(std::string name, std::size_t depth)
      : ConfigBaseClonable(Type::kStruct, name, depth){};

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

  ~ConfigStruct() noexcept override = default;

 protected:
  ConfigStruct(const ConfigStruct&) = default;
  ConfigStruct& operator=(const ConfigStruct&) = delete;
};

class ConfigProto : public ConfigBaseClonable<ConfigStructLike, ConfigProto> {
 public:
  ConfigProto(std::string name, std::size_t depth)
      : ConfigBaseClonable(Type::kProto, name, depth) {}

  void stream(std::ostream& os) const override {
    const auto ws = std::string(depth * tw, ' ');
    os << ws << "proto " << name << " {\n";
#if DEBUG_CLASSES
    os << ws << "-- " << proto_vars.size() << " proto vars\n";
    os << ws << "-- " << data.size() << " k/v pairs\n";
#endif
    pprint(os, data, depth + 1);
    os << ws << "}";
  }

  // TODO: Think about this more. What if a var is used more than once?
  std::map<std::shared_ptr<ConfigVar>, std::string> proto_vars{};

  ~ConfigProto() noexcept override = default;

 protected:
  ConfigProto(const ConfigProto&) = default;
  ConfigProto& operator=(const ConfigProto&) = delete;
};

class ConfigReference : public ConfigBaseClonable<ConfigStructLike, ConfigReference> {
 public:
  ConfigReference(std::string name, std::string proto_name, std::size_t depth)
      : ConfigBaseClonable(Type::kReference, name, depth), proto{std::move(proto_name)} {};

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

 protected:
  ConfigReference(const ConfigReference&) = default;
  ConfigReference& operator=(const ConfigReference&) = delete;
};

};  // namespace config::types

template <>
struct fmt::formatter<config::types::Type> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>
  template <typename FormatContext>
  auto format(const config::types::Type& type, FormatContext& ctx) {
    const auto type_s = magic_enum::enum_name<config::types::Type>(type);
    return formatter<std::string_view>::format(type_s, ctx);
  }
};

template <>
struct fmt::formatter<std::shared_ptr<config::types::ConfigBase>> : formatter<std::string> {
  // parse is inherited from formatter<string_view>
  template <typename FormatContext>
  auto format(const std::shared_ptr<config::types::ConfigBase>& cfg, FormatContext& ctx) {
    std::stringstream ss;
    cfg->stream(ss);
    return formatter<std::string>::format(ss.str(), ctx);
  }
};
