include(CTest)
include(GoogleTest)

################################################################################
add_executable(
  utils_test
  utils_test.cpp
  )

target_link_libraries(
  utils_test
  flexi_cfg
  gtest_main
  )

target_include_directories(utils_test PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  )

add_clang_format(utils_test)
gtest_discover_tests(utils_test)

################################################################################
add_executable(
  config_helpers_test
  config_helpers_test.cpp
  )

target_link_libraries(
  config_helpers_test
  flexi_cfg
  fmt::fmt
  magic_enum::magic_enum
  range-v3::range-v3
  gtest_main
  )

target_include_directories(config_helpers_test PRIVATE
  ${PROJECT_SOURCE_DIR}/include/
  )

add_clang_format(config_helpers_test)
gtest_discover_tests(config_helpers_test)

################################################################################
add_executable(
  config_grammar_test
  config_grammar_test.cpp
  )

target_link_libraries(
  config_grammar_test
  flexi_cfg
  fmt::fmt
  magic_enum::magic_enum
  range-v3::range-v3
  taocpp::pegtl
  gtest_main
  )

target_include_directories(config_grammar_test PRIVATE
  ${PROJECT_SOURCE_DIR}/include/
  )

add_clang_format(config_grammar_test)
gtest_discover_tests(config_grammar_test)

################################################################################
add_executable(
  config_exception_test
  config_exception_test.cpp
  )

target_link_libraries(
  config_exception_test
  flexi_cfg
  taocpp::pegtl
  gtest_main
  )

target_include_directories(config_exception_test PRIVATE
  ${PROJECT_SOURCE_DIR}/include/
  )

add_clang_format(config_exception_test)
gtest_discover_tests(config_exception_test)

################################################################################
add_executable(
  config_parse_test
  config_parse_test.cpp
  )

target_link_libraries(
  config_parse_test
  flexi_cfg
  taocpp::pegtl
  gtest_main
  )

target_include_directories(config_parse_test PRIVATE
  ${PROJECT_SOURCE_DIR}/include/
  )

add_clang_format(config_parse_test)
gtest_discover_tests(config_parse_test)

################################################################################
add_executable(
  math_test
  math_test.cpp
  )

target_link_libraries(
  math_test
  flexi_cfg
  taocpp::pegtl
  fmt::fmt
  magic_enum::magic_enum
  gtest_main
  )

target_include_directories(math_test PRIVATE
  ${PROJECT_SOURCE_DIR}/include/
  )

add_clang_format(math_test)
gtest_discover_tests(math_test)
