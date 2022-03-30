#include <any>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "config_helpers.h"

#define DEBUG_CLASSES 0

template <typename Key, typename Value>
inline std::ostream& operator<<(std::ostream& os, const std::map<Key, Value>& data) {
  for (const auto& kv : data) {
    os << kv.first << " = " << kv.second << "\n";
  }
  return os;
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
};

inline std::ostream& operator<<(std::ostream& os, const ConfigBase& cfg) {
  cfg.stream(os);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<ConfigBase>& cfg) {
  return (cfg ? (os << *cfg) : (os << "NULL"));
}

template <typename Key, typename Value>
inline std::ostream& operator<<(std::ostream& os, const std::map<Key, Value>& data) {
  for (const auto& kv : data) {
    os << kv.first << " = " << kv.second << "\n";
  }
  return os;
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
      : ConfigBase(Type::kValueLookup), keys{config::utils::split(var_ref, '.')} {};

  void stream(std::ostream& os) const override { os << "$(" << var() << ")"; }

  const std::vector<std::string> keys{};

  auto var() const -> std::string { return config::utils::join(keys, "."); }
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

inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<ConfigVar>& cfg) {
  return (cfg ? (os << *cfg) : (os << "NULL"));
}

class ConfigStructLike : public ConfigBase {
 public:
  ConfigStructLike(Type in_type, std::string name) : ConfigBase(in_type), name{std::move(name)} {};

  void stream(std::ostream& os) const override {
    os << "struct-like " << name << "\n";
    os << data;
  }

  const std::string name{};

  std::map<std::string, std::shared_ptr<ConfigBase>> data;
};

class ConfigStruct : public ConfigStructLike {
 public:
  ConfigStruct(std::string name) : ConfigStructLike(Type::kStruct, name){};

  void stream(std::ostream& os) const override {
    os << "struct " << name << " {\n";
#if DEBUG_CLASSES
    os << "  " << data.size() << " k/v pairs\n";
#endif
    os << data;
    os << "}";
  }
};

class ConfigProto : public ConfigStructLike {
 public:
  ConfigProto(std::string name) : ConfigStructLike(Type::kProto, name) {}

  void stream(std::ostream& os) const override {
    os << "!PROTO! " << name << " {\n";
#if DEBUG_CLASSES
    os << "  " << proto_vars.size() << " proto vars\n";
    os << "  " << data.size() << " k/v pairs\n";
#endif
    os << proto_vars << "\n";
    os << data;
    os << "}";
  }

  std::map<std::string, std::shared_ptr<ConfigVar>> proto_vars;
};

class ConfigReference : public ConfigStructLike {
 public:
  ConfigReference(std::string name, std::string proto_name)
      : ConfigStructLike(Type::kReference, name), proto{std::move(proto_name)} {};

  void stream(std::ostream& os) const override {
    os << "!REFERENCE! " << proto << " as " << name << " {\n";
#if DEBUG_CLASSES
    os << "  " << ref_vars.size() << " ref vars\n";
    os << "  " << data.size() << " k/v pairs\n";
#endif
    os << ref_vars << "\n";
    os << data;
    os << "}";
  }

  const std::string proto{};

  std::map<std::shared_ptr<ConfigVar>, std::shared_ptr<ConfigBase>> ref_vars;
};

};  // namespace config::types
