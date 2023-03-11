

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)

message(STATUS "pybind11 version: ${pybind11_VERSION}")
message(STATUS "Include dirs:  ${pybind11_INCLUDE_DIRS}")


