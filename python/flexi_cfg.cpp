#include <flexi_cfg/parser.h>
#include <flexi_cfg/reader.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include <filesystem>
#include <string_view>

namespace py = pybind11;

/*
py::class_<std::filesystem::path>(m, "Path")
    .def(py::init<std::string>());
py::implicitly_convertible<std::string, std::filesystem::path>();
*/

// Helper function avoids the need to be extra verbose with selecting overloads for 'getValue'
// methods
template <typename T>
auto getValueHelper(const flexi_cfg::Reader& cfg, const std::string& key) -> T {
  return cfg.getValue<T>(key);
}

PYBIND11_MODULE(py_flexi_cfg, m) {
  py::class_<flexi_cfg::Reader>(m, "Reader")
      .def("dump", &flexi_cfg::Reader::dump)
      .def("exists", &flexi_cfg::Reader::exists)
      .def("keys", &flexi_cfg::Reader::keys)
      .def("findStructWithKey", &flexi_cfg::Reader::findStructsWithKey)
      .def("getValue_int", &getValueHelper<int64_t>)
      .def("getValue_uint64", &getValueHelper<uint64_t>)
      .def("getValue_double", &getValueHelper<double>)
      .def("getValue_bool", &getValueHelper<bool>)
      .def("getValue_string", &getValueHelper<std::string>)
      .def("getValue_reader", &getValueHelper<flexi_cfg::Reader>);

  py::class_<flexi_cfg::Parser>(m, "Parser")
      .def("parse",
           py::overload_cast<std::string_view, std::string_view>(&flexi_cfg::Parser::parse),
           py::arg("source") = "unknown")
      .def("parse", py::overload_cast<const std::filesystem::path&>(&flexi_cfg::Parser::parse));
}