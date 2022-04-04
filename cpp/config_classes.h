#include <fmt/format.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "utils.h"

#define DEBUG_CLASSES 0
#define PRINT_SRC 0

namespace {
constexpr std::size_t tw{4};  // The width of the indentation
}

namespace config::types {

enum class Type { kStruct, kProto, kReference, kVar, kValueLookup, kValue, kUnknown };

// This is the base-class from which all config nodes shall derive
class ConfigBase {
 protected:
  explicit ConfigBase(const Type in_type) : type{in_type} {}
  virtual ~ConfigBase() = default;

 public:
  ConfigBase(const ConfigBase&) = delete;
  ConfigBase(ConfigBase&&) = delete;

  ConfigBase& operator=(const ConfigBase&) = delete;
  ConfigBase& operator=(ConfigBase&&) = delete;

  virtual void stream(std::ostream&) const = 0;

  const Type type;

  std::size_t line{0};
  std::string source{};
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
      os << "\n" << ws << kv.second << "\n\n";
    } else {
      os << ws << kv.first << " = " << kv.second
#if PRINT_SRC
         << "  # " << kv.second->source << ":" << kv.second->line
#endif
         << "\n";
    }
  }
}

class ConfigValue : public ConfigBase {
 public:
  ConfigValue(std::string value_in) : ConfigBase(Type::kValue), value{value_in} {};

  void stream(std::ostream& os) const override { os << value; }

  const std::string value{};
};

class ConfigValueLookup : public ConfigBase {
 public:
  ConfigValueLookup(const std::string& var_ref)
      : ConfigBase(Type::kValueLookup), keys{utils::split(var_ref, '.')} {};

  void stream(std::ostream& os) const override { os << "$(" << var() << ")"; }

  const std::vector<std::string> keys{};

  auto var() const -> std::string { return utils::join(keys, "."); }
};

class ConfigVar : public ConfigBase {
 public:
  ConfigVar(std::string name) : ConfigBase(Type::kVar), name{std::move(name)} {};

  void stream(std::ostream& os) const override { os << name; }

  const std::string name{};
};

inline std::ostream& operator<<(std::ostream& os, const ConfigVar& cfg) {
  cfg.stream(os);
  return os;
}

class ConfigStructLike : public ConfigBase {
 public:
  ConfigStructLike(Type in_type, std::string name, std::size_t depth)
      : ConfigBase(in_type), name{std::move(name)}, depth{depth} {};

  void stream(std::ostream& os) const override {
    os << "struct-like " << name << "\n";
    os << data;
  }

  const std::string name{};

  const std::size_t depth{};

  std::map<std::string, std::shared_ptr<ConfigBase>> data;
};

class ConfigStruct : public ConfigStructLike {
 public:
  ConfigStruct(std::string name, std::size_t depth)
      : ConfigStructLike(Type::kStruct, name, depth){};

  void stream(std::ostream& os) const override {
    os << "struct " << name << " {\n";
#if DEBUG_CLASSES
    os << "  " << data.size() << " k/v pairs\n";
#endif
    pprint(os, data, depth);
    // os << data;
    os << std::string((depth - 1) * tw, ' ') << "}";
  }
};

class ConfigProto : public ConfigStructLike {
 public:
  ConfigProto(std::string name, std::size_t depth) : ConfigStructLike(Type::kProto, name, depth) {}

  void stream(std::ostream& os) const override {
    os << "proto " << name << " {\n";
#if DEBUG_CLASSES
    os << "  " << proto_vars.size() << " proto vars\n";
    os << "  " << data.size() << " k/v pairs\n";
#endif
    pprint(os, proto_vars, depth);
    pprint(os, data, depth);
    // os << proto_vars << "\n";
    // os << data;
    os << std::string((depth - 1) * tw, ' ') << "}";
  }

  std::map<std::string, std::shared_ptr<ConfigVar>> proto_vars;
};

class ConfigReference : public ConfigStructLike {
 public:
  ConfigReference(std::string name, std::string proto_name, std::size_t depth)
      : ConfigStructLike(Type::kReference, name, depth), proto{std::move(proto_name)} {};

  void stream(std::ostream& os) const override {
    os << "reference " << proto << " as " << name << " {\n";
#if DEBUG_CLASSES
    os << "  " << ref_vars.size() << " ref vars\n";
    os << "  " << data.size() << " k/v pairs\n";
#endif
    pprint(os, ref_vars, depth);
    pprint(os, data, depth);
    // os << ref_vars << "\n";
    // os << data;
    os << std::string((depth - 1) * tw, ' ') << "}";
  }

  const std::string proto{};

  std::map<std::shared_ptr<ConfigVar>, std::shared_ptr<ConfigBase>> ref_vars;
};

};  // namespace config::types

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
