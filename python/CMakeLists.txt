
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


# detect virtualenv and set Pip args accordingly
if(DEFINED ENV{VIRTUAL_ENV} OR DEFINED ENV{CONDA_PREFIX})
  set(_pip_args)
else()
  set(_pip_args "--user")
endif()


if (NOT(${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  # install pybind11-stubgen via pip
  execute_process(COMMAND ${Python_EXECUTABLE} -m pip install ${_pip_args} "pybind11-stubgen")
  add_custom_command(
    OUTPUT ${PROJECT_NAME}/__init__.pyi
    COMMAND export PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}
    COMMAND pybind11-stubgen --root-suffix '' -o ${CMAKE_CURRENT_BINARY_DIR} flexi_cfg
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS flexi_cfg_py
  )

  add_custom_target(flexi_cfg_py_stubgen ALL DEPENDS ${PROJECT_NAME}/__init__.pyi)
endif()


install(TARGETS flexi_cfg_py
        COMPONENT python
        LIBRARY DESTINATION "${CFG_PYTHON_INSTALL_DIR}")
# Install the stubs file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/__init__.pyi
        COMPONENT python
        DESTINATION "${CFG_PYTHON_INSTALL_DIR}/${PROJECT_NAME}")
