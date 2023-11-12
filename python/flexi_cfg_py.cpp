#include <flexi_cfg/config/classes.h>
#include <flexi_cfg/config/exceptions.h>
#include <flexi_cfg/config/helpers.h>
#include <flexi_cfg/parser.h>
#include <flexi_cfg/reader.h>
#include <flexi_cfg/utils.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include <filesystem>
#include <string_view>

namespace py = pybind11;

namespace flexi_cfg {
class ReaderCfgMapAccessor : public Reader {
 public:
  explicit ReaderCfgMapAccessor(const Reader& reader) : Reader(reader) {}

  using ListPtr = std::shared_ptr<config::types::ConfigList>;

  template <typename T>
  [[nodiscard]] auto getList(const std::string& key) const -> py::list {
    const auto keys = utils::split(key, '.');
    const auto& cfg_val = config::helpers::getConfigValue(getCfgMap(), keys);
    // Ensure this is a list if the user is asking for a list.
    if (cfg_val->type != config::types::Type::kList) {
      THROW_EXCEPTION(config::InvalidTypeException,
                      "Expected '{}' to contain a list, but is of type {}", key, cfg_val->type);
    }

    // We have a list. Now we need to walk the list and collect the values, stuffing them into a
    // python list as we go.
    py::list list;
    listBuilder<T>(dynamic_pointer_cast<config::types::ConfigValue>(cfg_val), list);

    return list;
  }

 protected:
  template <typename T>
  void listBuilder(const config::types::ValuePtr& cfg_val, py::list& py_list) const {
    const auto& cfg_list_ptr = dynamic_pointer_cast<config::types::ConfigList>(cfg_val);
    if (cfg_list_ptr == nullptr) {
      THROW_EXCEPTION(config::InvalidTypeException, "Expected '{}' type but got '{}' type.",
                      config::types::Type::kList, cfg_val->type);
    }
    // We have a list. Now we need to walk the list and collect the values, stuffing them into a
    // python list as we go.
    if (cfg_list_ptr == nullptr) {
      THROW_EXCEPTION(config::InvalidStateException, "Expected '{}' type but got nullptr.",
                      config::types::Type::kList);
    }

    for (const auto& e : cfg_list_ptr->data) {
      const auto& list_value_ptr = dynamic_pointer_cast<config::types::ConfigValue>(e);
      if (e->type == config::types::Type::kList) {
        py::list new_list;
        listBuilder<T>(list_value_ptr, new_list);
        py_list.append(new_list);
      } else {
        T v{};
        convert(list_value_ptr, v);
        py_list.append(v);
      }
    }
  }
};

}  // namespace flexi_cfg

// Helper function avoids the need to be extra verbose with selecting overloads for 'getValue'
// methods
template <typename T>
auto getValueHelper(const flexi_cfg::Reader& cfg, const std::string& key) -> T {
  return cfg.getValue<T>(key);
}

template <typename T>
auto getListHelper(const flexi_cfg::Reader& cfg, const std::string& key) -> py::list {
  const auto& map_accesor = flexi_cfg::ReaderCfgMapAccessor(cfg);
  return map_accesor.getList<T>(key);
}

auto getValueGeneric(const flexi_cfg::Reader& cfg, const std::string& key) -> py::object {
  const auto key_type = cfg.getType(key);
  switch (key_type) {
    case flexi_cfg::config::types::Type::kString:
      return py::cast(cfg.getValue<std::string>(key));
    case flexi_cfg::config::types::Type::kBoolean:
      return py::cast(cfg.getValue<bool>(key));
    case flexi_cfg::config::types::Type::kNumber:
      try {
        return py::cast(cfg.getValue<uint64_t>(key));
      } catch (const flexi_cfg::config::MismatchTypeException& e) {
        // Ignore and try the next type
        try {
          return py::cast(cfg.getValue<int64_t>(key));
        } catch (const flexi_cfg::config::MismatchTypeException& e) {
          return py::cast(cfg.getValue<double>(key));
        }
      }
    case flexi_cfg::config::types::Type::kList:
      try {
        return getListHelper<std::string>(cfg, key);
      } catch (const flexi_cfg::config::MismatchTypeException& e) {
        // Ignore and try the next type
        try {
          return getListHelper<bool>(cfg, key);
        } catch (const flexi_cfg::config::MismatchTypeException& e) {
          // Ignore and try the next type
          try {
            return getListHelper<uint64_t>(cfg, key);
          } catch (const flexi_cfg::config::MismatchTypeException& e) {
            // Ignore and try the next type
            try {
              return getListHelper<int64_t>(cfg, key);
            } catch (const flexi_cfg::config::MismatchTypeException& e) {
              return getListHelper<double>(cfg, key);
            }
          }
        }
      }
    case flexi_cfg::config::types::Type::kStruct:
      return py::cast(cfg.getValue<flexi_cfg::Reader>(key));
    default:
      THROW_EXCEPTION(flexi_cfg::config::InvalidTypeException,
                      "Unsupported type '{}' generic 'getValue'", key_type);
  }
}

// We need something to associate the enum with aside from the module, so create this empty struct
struct Type {};
// We need something to associate the logger with aside from the module.
struct Logger {};

PYBIND11_MODULE(flexi_cfg, m) {
  m.doc() = "flexi_cfg python bindings";

  py::class_<Type> type_holder(m, "Type");
  py::enum_<flexi_cfg::config::types::Type>(type_holder, "Type")
      .value("VALUE", flexi_cfg::config::types::Type::kValue)
      .value("STRING", flexi_cfg::config::types::Type::kString)
      .value("NUMBER", flexi_cfg::config::types::Type::kNumber)
      .value("BOOLEAN", flexi_cfg::config::types::Type::kBoolean)
      .value("LIST", flexi_cfg::config::types::Type::kList)
      .value("EXPRESSION", flexi_cfg::config::types::Type::kExpression)
      .value("VALUE_LOOKUP", flexi_cfg::config::types::Type::kValueLookup)
      .value("VAR", flexi_cfg::config::types::Type::kVar)
      .value("STRUCT", flexi_cfg::config::types::Type::kStruct)
      .value("STRUCT_IN_PROTO", flexi_cfg::config::types::Type::kStructInProto)
      .value("PROTO", flexi_cfg::config::types::Type::kProto)
      .value("REFERENCE", flexi_cfg::config::types::Type::kReference)
      .value("UNKNOWN", flexi_cfg::config::types::Type::kUnknown)
      .export_values();

  auto pyFlexiCfgException =
      py::register_exception<flexi_cfg::config::Exception>(m, "Exception", PyExc_RuntimeError);
  py::register_exception<flexi_cfg::config::InvalidTypeException>(m, "InvalidTypeException",
                                                                  pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::InvalidStateException>(m, "InvalidStateException",
                                                                   pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::InvalidConfigException>(m, "InvalidConfigException",
                                                                    pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::InvalidKeyException>(m, "InvalidKeyException",
                                                                 pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::DuplicateKeyException>(m, "DuplicateKeyException",
                                                                   pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::MismatchKeyException>(m, "MismatchKeyException",
                                                                  pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::MismatchTypeException>(m, "MismatchTypeException",
                                                                   pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::UndefinedReferenceVarException>(
      m, "UndefinedReferenceVarException", pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::UndefinedProtoException>(m, "UndefinedProtoException",
                                                                     pyFlexiCfgException);
  py::register_exception<flexi_cfg::config::CyclicReferenceException>(m, "CyclicReferenceException",
                                                                      pyFlexiCfgException);

  py::class_<flexi_cfg::Reader>(m, "Reader")
      .def("dump", &flexi_cfg::Reader::dump)
      .def("exists", &flexi_cfg::Reader::exists)
      .def("keys", &flexi_cfg::Reader::keys)
      .def("get_type", &flexi_cfg::Reader::getType)
      .def("find_structs_with_key", &flexi_cfg::Reader::findStructsWithKey)
      // Accessors for single values
      .def("get_int", &getValueHelper<int64_t>)
      .def("get_uint64", &getValueHelper<uint64_t>)
      .def("get_float", &getValueHelper<double>)
      .def("get_bool", &getValueHelper<bool>)
      .def("get_string", &getValueHelper<std::string>)
      // Accessors for lists of values
      .def("get_int_list", &getListHelper<int64_t>)
      .def("get_uint64_list", &getListHelper<uint64_t>)
      .def("get_float_list", &getListHelper<double>)
      .def("get_bool_list", &getListHelper<bool>)
      .def("get_string_list", &getListHelper<std::string>)
      // Accessor for a sub-reader object
      .def("get_reader", &getValueHelper<flexi_cfg::Reader>)
      // Generic accessor
      .def("get_value", &getValueGeneric);

  py::class_<flexi_cfg::Parser>(m, "Parser")
      .def_static("parse", &flexi_cfg::Parser::parse, py::arg("cfg_file"), py::arg("root_dir") = std::nullopt)
      .def_static("parse_from_string",
                  &flexi_cfg::Parser::parseFromString,
                  py::arg("cfg_string"), py::pos_only(), py::arg("source") = "unknown");

  m.def("parse", &flexi_cfg::parse, py::arg("cfg_file"), py::arg("root_dir") = std::nullopt);
  m.def("parse_from_string", &flexi_cfg::parseFromString,
        py::arg("cfg_string"), py::pos_only(), py::arg("source") = "unknown");

  py::class_<Logger> logger_holder(m, "logger");
  py::enum_<flexi_cfg::logger::Severity>(logger_holder, "Severity")
      .value("TRACE", flexi_cfg::logger::Severity::TRACE)
      .value("DEBUG", flexi_cfg::logger::Severity::DEBUG)
      .value("INFO", flexi_cfg::logger::Severity::INFO)
      .value("WARN", flexi_cfg::logger::Severity::WARN)
      .value("ERROR", flexi_cfg::logger::Severity::ERROR)
      .value("CRITICAL", flexi_cfg::logger::Severity::CRITICAL);
  logger_holder.def("set_level", &flexi_cfg::logger::setLevel);
  logger_holder.def("log_level", &flexi_cfg::logger::logLevel);
}
