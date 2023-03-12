

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)

message(STATUS "pybind11 version: ${pybind11_VERSION}")
message(STATUS "Include dirs:  ${pybind11_INCLUDE_DIRS}")

pybind11_add_module(py_flexi_cfg flexi_cfg.cpp)
target_link_libraries(py_flexi_cfg PUBLIC flexi_cfg)
set_target_properties(py_flexi_cfg PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

