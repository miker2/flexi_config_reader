cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

project (FLEXI_CONFIG_READER)


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


add_definitions(-DEXAMPLE_DIR="${CMAKE_SOURCE_DIR}/examples")

add_subdirectory(cpp)
