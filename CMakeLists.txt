cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

project (flexi_cfg LANGUAGES CXX)

set(CFG_IS_MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CFG_IS_MAIN_PROJECT ON)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/colours.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/add_clang_format.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

include(FetchContent)
# Get taocpp::pegtl
FetchContent_Declare(
  taocpp_pegtl
  GIT_REPOSITORY  https://github.com/taocpp/PEGTL.git
  GIT_TAG         3.2.5
  )
FetchContent_MakeAvailable(taocpp_pegtl)

# Get gtest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.11.0
  )
FetchContent_MakeAvailable(googletest)

# magic_enum tag v0.7.3
FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.7.3
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
  GIT_TAG d800a032132512a54c291ce55a2a43e0460591c7  # 0.11.0 is broken on clang-13, but master works
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

add_definitions(-DEXAMPLE_DIR="${CMAKE_SOURCE_DIR}/examples")

file(GLOB CFG_HEADERS include/*.h)

add_library(flexi_cfg
  ${CFG_HEADERS}
  src/config_helpers.cpp
  src/config_reader.cpp
  src/math_helpers.cpp
)
add_library(flexi_cfg::flexi_cfg ALIAS flexi_cfg)
target_include_directories(flexi_cfg PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(flexi_cfg
  taocpp::pegtl
  magic_enum::magic_enum
  fmt::fmt
  range-v3::range-v3
)
add_clang_format(flexi_cfg)

install(TARGETS flexi_cfg
  LIBRARY DESTINATION lib
  PUBLIC_HEADER DESTINATION include)


add_subdirectory(src)

if (CFG_ENABLE_TEST)
  enable_testing()
  add_subdirectory(tests)
endif()
