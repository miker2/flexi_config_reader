cmake_minimum_required(VERSION 3.14)

set(CFG_IS_MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CFG_IS_MAIN_PROJECT ON)
endif()

project (flexi_cfg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_SOURCE_DIR}/cmake/colours.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/add_clang_format.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

include(FetchContent)
# Get taocpp::pegtl
FetchContent_Declare(
  taocpp_pegtl
  GIT_REPOSITORY  https://github.com/taocpp/PEGTL.git
  GIT_TAG         3.2.7
  )
FetchContent_MakeAvailable(taocpp_pegtl)

# magic_enum tag v0.7.3
FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.8.1
  )
FetchContent_MakeAvailable(magic_enum)

# FMT tag 8.1.1
set(FMT_VERSION_REQUIRED 8.1.1)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
  GIT_TAG ${FMT_VERSION_REQUIRED}
  CMAKE_ARGS -DFMT_DOC=OFF -DFMT_TEST=OFF -DFMT_INSTALL=ON -DFMT_LIB_DIR=lib
)
FetchContent_MakeAvailable(fmt)

# range-v3
FetchContent_Declare(
  range-v3
  GIT_REPOSITORY "https://github.com/ericniebler/range-v3.git"
  GIT_TAG 0.12.0
)
FetchContent_MakeAvailable(range-v3)

option(ENABLE_CLANG_TIDY "Enable static analysis with clang-tidy" ON)
if(ENABLE_CLANG_TIDY)
  if(NOT CMAKE_CROSSCOMPILING) # adds invalid paths
    find_package(ClangTools QUIET)
    if(CLANG_TIDY_FOUND)
      set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_BIN}")
    else(CLANG_TIDY_FOUND)
      message(STATUS "${Yellow}clang-tidy static analysis won't be done.${ColourReset}")
    endif(CLANG_TIDY_FOUND)
    if(NOT CLANG_FORMAT_FOUND)
      message(STATUS "${Yellow}clang-format not found. Style checks won't be done.${ColourReset}")
    endif(NOT CLANG_FORMAT_FOUND)
  endif(NOT CMAKE_CROSSCOMPILING)
else(ENABLE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY "")
endif(ENABLE_CLANG_TIDY)

option(CFG_ENABLE_TEST "Enable unit tests." ON)
option(CFG_EXAMPLES "Build example applications." OFF)

add_definitions(-DEXAMPLE_DIR="${CMAKE_SOURCE_DIR}/examples")

set(PUBLIC_CFG_HEADERS include/flexi_cfg/config/reader.h)
set(CFG_HEADERS
  ${PUBLIC_CFG_HEADERS}
  include/flexi_cfg/config/actions.h
  include/flexi_cfg/config/classes.h
  include/flexi_cfg/config/exceptions.h
  include/flexi_cfg/config/grammar.h
  include/flexi_cfg/config/helpers.h
  include/flexi_cfg/config/selector.h
  include/flexi_cfg/logger.h
  include/flexi_cfg/math/actions.h
  include/flexi_cfg/math/grammar.h
  include/flexi_cfg/math/helpers.h
  include/flexi_cfg/utils.h)

add_library(flexi_cfg
  ${CFG_HEADERS}
  src/config_helpers.cpp
  src/config_reader.cpp
  src/math_helpers.cpp
)
add_library(flexi_cfg::flexi_cfg ALIAS flexi_cfg)
target_include_directories(flexi_cfg PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
target_compile_features(flexi_cfg INTERFACE cxx_std_20)
set_target_properties(flexi_cfg PROPERTIES PUBLIC_HEADER "${PUBLIC_CFG_HEADERS}")

target_link_libraries(flexi_cfg
  taocpp::pegtl
  magic_enum::magic_enum
  fmt::fmt
  range-v3::range-v3
)
add_clang_format(flexi_cfg)

install(TARGETS flexi_cfg
  LIBRARY DESTINATION lib
  PUBLIC_HEADER DESTINATION include/flexi_cfg)


add_subdirectory(src)

if (CFG_ENABLE_TEST)
  # Get gtest
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.11.0
    )
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_subdirectory(tests)
endif()
