
find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)

message(STATUS "pybind11 found: x${pybind11_FOUND}x")
message(STATUS "pybind11 version: x${pybind11_VERSION}x")
message(STATUS "Include dirs:  ${pybind11_INCLUDE_DIRS}")
message(STATUS "Libraries:  x${pybind11_LIBRARIES}x")
message(STATUS "site-packages (arch):  ${Python_SITEARCH}")
message(STATUS "site-packages (lib):   ${Python_SITELIB}")
message(STATUS "Python version:  x${Python_VERSION}x")
message(STATUS "SITE_PACKAGES:  x${Python_SITE_PACKAGES}x")

if (NOT CFG_PYTHON_INSTALL_DIR)
	set(CFG_PYTHON_INSTALL_DIR "${Python_SITELIB}")
endif ()

message(STATUS "Installing python bindings to: ${CFG_PYTHON_INSTALL_DIR}")



pybind11_add_module(flexi_cfg_py flexi_cfg_py.cpp)
target_link_libraries(flexi_cfg_py PUBLIC flexi_cfg)
set_target_properties(flexi_cfg_py PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(flexi_cfg_py PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

file(GLOB CFG_PURE_PYTHON RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.py")

message(STATUS "pure python: x${CFG_PURE_PYTHON}x")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${CFG_PURE_PYTHON} "." COPYONLY)

message(STATUS "Target objects:  x$<TARGET_OBJECTS:flexi_cfg_py>x")
message(STATUS "Target file:     x$<TARGET_FILE:flexi_cfg_py>x")
message(STATUS "Output name:     x$<OUTPUT_NAME:flexi_cfg_py>")
message(STATUS "Lib output name: x$<LIBRARY_OUTPUT_NAME:flexi_cfg_py>")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CFG_PURE_PYTHON}
	            #${CMAKE_CURRENT_BINARY_DIR}/$<LIBRARY_OUTPUT_NAME:flexi_cfg_py>
	      DESTINATION ${CFG_PYTHON_INSTALL_DIR}/${PROJECT_NAME})
