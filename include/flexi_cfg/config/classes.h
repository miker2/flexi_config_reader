#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <any>
#include <iosfwd>
#include <iterator>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "flexi_cfg/details/ordered_map.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/utils.h"

#define DEBUG_CLASSES 0

namespace flexi_cfg::config::types {
constexpr std::size_t tw{4};  // The width of the indentation

class ConfigBase;
using BasePtr = std::shared_ptr<ConfigBase>;

/// \brief Renders `cfg` with default options, at indent level 0.
///
/// Declared here so the `stream()` shim below can call it; defined in `flexi_cfg/render-ostream.h`,
/// which this header includes at the bottom once the node types are complete.
void renderToStream(std::ostream& os, const ConfigBase& cfg);

using CfgMap = details::ordered_map<std::string, BasePtr, details::string_hash>;
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

  /// \brief Renders this node with default options. Kept for backward compatibility; the
  /// configurable entry points are `flexi_cfg::Dump` and `Reader::dump(os, opts)`.
  virtual void stream(std::ostream& os) const { renderToStream(os, *this); }

  [[nodiscard]] virtual auto clone() const -> BasePtr = 0;

  [[nodiscard]] auto loc() const -> std::string {
    std::string s = fmt::format("{}:{}", source, line);
    // Only report origins that add information: skip any that point back at this node's own
    // location, and collapse consecutive repeats of the same location.
    const auto same_loc = [](const ConfigBase& a, const ConfigBase& b) -> bool {
      return a.line == b.line && a.source == b.source;
    };
    const ConfigBase* last = nullptr;
    for (const auto& origin : origins) {
      if (same_loc(*origin, *this) || (last != nullptr && same_loc(*origin, *last))) {
        continue;
      }
      fmt::format_to(std::back_inserter(s), "{}{}:{}", last == nullptr ? " (from " : " <- ",
                     origin->source, origin->line);
      last = origin.get();
    }
    if (last != nullptr) {
      s += ")";
    }
    return s;
  }

  const Type type;

  std::size_t line{0};
  std::string source{};
  std::vector<std::shared_ptr<ConfigBase>> origins{};

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
    // The object is a Derived (CRTP), never a make_shared_enabler, so it must
    // not be cast to one; UBSan flags that as an invalid downcast. Instead let
    // the enabler copy-construct its Derived base from *this.
    struct make_shared_enabler : public Derived {
      explicit make_shared_enabler(const Derived& d) : Derived(d) {}
    };
    return std::make_shared<make_shared_enabler>(static_cast<const Derived&>(*this));
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

// Concept for map-like containers that can be streamed
template <typename T>
concept MapLike = requires(const T& map) {
  typename T::key_type;
  typename T::mapped_type;
  { map.begin() } -> std::input_iterator;
  { map.end() } -> std::input_iterator;
  requires requires(typename T::const_iterator it) {
    { it->first } -> std::convertible_to<const typename T::key_type&>;
    { it->second } -> std::convertible_to<const typename T::mapped_type&>;
  };
};

template <typename T>
concept SharedPtrMap =
    MapLike<T> &&
    std::is_same_v<typename T::mapped_type, std::shared_ptr<typename T::mapped_type::element_type>>;

class ConfigValue : public ConfigBaseClonable<ConfigBase, ConfigValue> {
 public:
  explicit ConfigValue(std::string value_in, Type type, std::any val = {})
      : ConfigBaseClonable(type), value{std::move(value_in)}, value_any{std::move(val)} {};

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
  ConfigList(std::string value_in = "") : ConfigBaseClonable(std::move(value_in), Type::kList) {};

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
  ConfigStructLike(Type in_type, std::string name)
      : ConfigBaseClonable(in_type), name{std::move(name)} {};

  const std::string name{};

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
  explicit ConfigStruct(std::string name, Type type = Type::kStruct)
      : ConfigBaseClonable(type, std::move(name)) {};

  [[nodiscard]] auto clone() const -> BasePtr final {
    // This sort of feels like a dirty hack, but appears to work.
    // See: https://stackoverflow.com/a/25069711
    // See ConfigBaseClonable::clone for why the enabler is built from the
    // base rather than *this being cast to the enabler type.
    struct make_shared_enabler : public ConfigStruct {
      explicit make_shared_enabler(const ConfigStruct& c) : ConfigStruct(c) {}
    };
    auto cloned = std::make_shared<make_shared_enabler>(*this);
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
  explicit ConfigProto(std::string name) : ConfigBaseClonable(Type::kProto, std::move(name)) {}

  [[nodiscard]] auto clone() const -> BasePtr final {
    // This sort of feels like a dirty hack, but appears to work.
    // See: https://stackoverflow.com/a/25069711
    // See ConfigBaseClonable::clone for why the enabler is built from the
    // base rather than *this being cast to the enabler type.
    struct make_shared_enabler : public ConfigProto {
      explicit make_shared_enabler(const ConfigProto& c) : ConfigProto(c) {}
    };
    auto cloned = std::make_shared<make_shared_enabler>(*this);
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
  ConfigReference(const std::string& name, std::string proto_name)
      : ConfigBaseClonable(Type::kReference, name), proto{std::move(proto_name)} {
    // Create the required key to easily reference the parent name.
    // NOTE: Its location is filled in by 'action<REFs>', as it isn't known yet at this point.
    ref_vars["$PARENT_NAME"] = std::make_shared<ConfigValue>(name, Type::kString);
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

// Pull in the renderer and its ostream/fmt adapters now that the node types above are complete.
// This has to be the last thing in the header: `render.h` includes this one straight back, so
// anything placed below would be skipped on the path that starts at `render.h`.
#include "flexi_cfg/render.h"  // NOLINT(misc-include-cleaner,llvm-include-order)
