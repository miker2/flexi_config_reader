message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Version:  ${CMAKE_CXX_COMPILER_VERSION}")

add_executable(jsonish_test jsonish_test.cpp)
target_link_libraries(jsonish_test PRIVATE taocpp::pegtl)


add_library(config_reader
  STATIC
  ${CFG_HEADERS}
  config_helpers.cpp
  config_reader.cpp
  math_helpers.cpp
)

target_link_libraries(config_reader
  taocpp::pegtl
  magic_enum::magic_enum
  fmt::fmt
  range-v3::range-v3
)

target_include_directories(config_reader PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_clang_format(config_reader)

install(TARGETS config_reader
  LIBRARY DESTINATION lib
  PUBLIC_HEADER DESTINATION include)

add_executable(config_build config_build.cpp)
target_link_libraries(config_build
  PRIVATE
  config_reader
)
target_include_directories(config_build PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
add_clang_format(config_build)

add_executable(config_reader_example config_reader_example.cpp)
target_link_libraries(config_reader_example
  PRIVATE
  config_reader
  fmt::fmt
)
target_include_directories(config_reader_example PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
add_clang_format(config_reader_example)

add_executable(include_file_test include_file_test.cpp)
target_link_libraries(include_file_test PRIVATE taocpp::pegtl)
target_include_directories(include_file_test PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)


install(TARGETS jsonish_test config_build config_reader_example include_file_test
        DESTINATION bin)
