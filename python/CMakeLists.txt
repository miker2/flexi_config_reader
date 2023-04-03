
find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)

# message(STATUS "site-packages (arch):  ${Python_SITEARCH}")
# message(STATUS "site-packages (lib):   ${Python_SITELIB}")
# message(STATUS "Python version:  x${Python_VERSION}x")
# message(STATUS "Python version: x${Python_VERSION_MAJOR}x.x${Python_VERSION_MINOR}")

if (NOT CFG_PYTHON_INSTALL_DIR)
  set(CFG_PYTHON_INSTALL_DIR "${Python_SITEARCH}")
endif ()

pybind11_add_module(flexi_cfg_py flexi_cfg_py.cpp)
target_link_libraries(flexi_cfg_py PUBLIC flexi_cfg)
set_target_properties(flexi_cfg_py PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(flexi_cfg_py PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

add_test(NAME test_flexi_cfg
  COMMAND ${Python_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
set_tests_properties(test_flexi_cfg
	PROPERTIES ENVIRONMENT "EXAMPLES_DIR=${PROJECT_SRC_DIR}/examples")

install(TARGETS flexi_cfg_py
        COMPONENT python
        LIBRARY DESTINATION "${CFG_PYTHON_INSTALL_DIR}")
