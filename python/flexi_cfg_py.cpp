#include <flexi_cfg/config/classes.h>
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
  ReaderCfgMapAccessor(const Reader& reader) : Reader(reader) {}

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
      THROW_EXCEPTION(std::runtime_error, "Expected '{}' type but got '{}' type.",
                      config::types::Type::kList, cfg_val->type);
    }
    // We have a list. Now we need to walk the list and collect the values, stuffing them into a
    // python list as we go.
    if (cfg_list_ptr == nullptr) {
      THROW_EXCEPTION(std::runtime_error, "Expected '{}' type but got nullptr.",
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

// We need something to associate the enum with aside from the module, so create this empty struct
struct Type {};

PYBIND11_MODULE(flexi_cfg_py, m) {
  py::class_<flexi_cfg::Reader>(m, "Reader")
      .def("dump", &flexi_cfg::Reader::dump)
      .def("exists", &flexi_cfg::Reader::exists)
      .def("keys", &flexi_cfg::Reader::keys)
      .def("findStructWithKey", &flexi_cfg::Reader::findStructsWithKey)
      // Accessors for single values
      .def("getValue_int", &getValueHelper<int64_t>)
      .def("getValue_uint64", &getValueHelper<uint64_t>)
      .def("getValue_double", &getValueHelper<double>)
      .def("getValue_bool", &getValueHelper<bool>)
      .def("getValue_string", &getValueHelper<std::string>)
      // Accessors for lists of values
      .def("getValue_list_int", &getListHelper<int64_t>)
      .def("getValue_list_uint64", &getListHelper<uint64_t>)
      .def("getValue_list_double", &getListHelper<double>)
      .def("getValue_list_bool", &getListHelper<bool>)
      .def("getValue_list_string", &getListHelper<std::string>)
      // Accessor for a sub-reader object
      .def("getValue_reader", &getValueHelper<flexi_cfg::Reader>);

  py::class_<Type> type_holder(m, "Type");
  py::enum_<flexi_cfg::config::types::Type>(type_holder, "Type")
      .value("kValue", flexi_cfg::config::types::Type::kValue)
      .value("kString", flexi_cfg::config::types::Type::kString)
      .value("kNumber", flexi_cfg::config::types::Type::kNumber)
      .value("kBoolean", flexi_cfg::config::types::Type::kBoolean)
      .value("kList", flexi_cfg::config::types::Type::kList)
      .value("kExpression", flexi_cfg::config::types::Type::kExpression)
      .value("kValueLookup", flexi_cfg::config::types::Type::kValueLookup)
      .value("kVar", flexi_cfg::config::types::Type::kVar)
      .value("kStruct", flexi_cfg::config::types::Type::kStruct)
      .value("kStructInProto", flexi_cfg::config::types::Type::kStructInProto)
      .value("kProto", flexi_cfg::config::types::Type::kProto)
      .value("kReference", flexi_cfg::config::types::Type::kReference)
      .value("kUnknown", flexi_cfg::config::types::Type::kUnknown)
      .export_values();

  m.def("parse", py::overload_cast<const std::filesystem::path&>(&flexi_cfg::parse));
  m.def("parse",
           py::overload_cast<std::string_view, std::string_view>(&flexi_cfg::parse),
           py::arg("cfg_string"), py::pos_only(), py::arg("source") = "unknown");
}