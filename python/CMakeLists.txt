

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)

message(STATUS "pybind11 version: ${pybind11_VERSION}")
message(STATUS "Include dirs:  ${pybind11_INCLUDE_DIRS}")

pybind11_add_module(flexi_cfg_py flexi_cfg_py.cpp)
target_link_libraries(flexi_cfg_py PUBLIC flexi_cfg)
set_target_properties(flexi_cfg_py PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

