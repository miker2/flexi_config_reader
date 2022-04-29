include(CTest)
include(GoogleTest)

################################################################################
add_executable(
  utils_test
  utils_test.cpp
  )

target_link_libraries(
  utils_test
  gtest_main
  )

target_include_directories(utils_test PRIVATE
  ${CMAKE_SOURCE_DIR}/cpp/
  )

gtest_discover_tests(utils_test)

################################################################################
add_executable(
  config_helpers_test
  config_helpers_test.cpp
  )

target_link_libraries(
  config_helpers_test
  config_reader
  fmt::fmt
  magic_enum::magic_enum
  range-v3::range-v3
  gtest_main
  )

target_include_directories(config_helpers_test PRIVATE
  ${CMAKE_SOURCE_DIR}/cpp/
  )

gtest_discover_tests(config_helpers_test)

################################################################################
add_executable(
  config_grammar_test
  config_grammar_test.cpp
  )

target_link_libraries(
  config_grammar_test
  fmt::fmt
  magic_enum::magic_enum
  range-v3::range-v3
  taocpp::pegtl
  gtest_main
  )

target_include_directories(config_grammar_test PRIVATE
  ${CMAKE_SOURCE_DIR}/cpp/
  )

gtest_discover_tests(config_grammar_test)

################################################################################
add_executable(
  math_test
  math_test.cpp
  )

target_link_libraries(
  math_test
  taocpp::pegtl
  fmt::fmt
  magic_enum::magic_enum
  gtest_main
  )

target_include_directories(math_test PRIVATE
  ${CMAKE_SOURCE_DIR}/cpp/
  )

gtest_discover_tests(math_test)
