message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Version:  ${CMAKE_CXX_COMPILER_VERSION}")

add_executable(jsonish_test jsonish_test.cpp)
target_link_libraries(jsonish_test PRIVATE taocpp::pegtl)

add_executable(toml_test toml_test.cpp)
target_link_libraries(toml_test PRIVATE taocpp::pegtl)


add_library(config_reader
  STATIC
  config_actions.h
  config_classes.h
  config_exceptions.h
  config_grammar.h
  config_helpers.h
  config_reader.h
  config_selector.h
  config_helpers.cpp
  config_reader.cpp
)

target_link_libraries(config_reader
  taocpp::pegtl
  magic_enum::magic_enum
  fmt::fmt
  range-v3::range-v3
)

target_include_directories(config_reader PRIVATE
  ${CMAKE_SOURCE_DIR}/cpp/
)

install(TARGETS config_reader
  LIBRARY DESTINATION lib
  PUBLIC_HEADER DESTINATION include)

add_executable(config_test config_test.cpp)
target_link_libraries(config_test
  PRIVATE
  config_reader
  taocpp::pegtl
)

add_executable(config_build config_build.cpp)
target_link_libraries(config_build
  PRIVATE
  config_reader
)

add_executable(config_reader_example config_reader_example.cpp)
target_link_libraries(config_reader_example
  PRIVATE
  config_reader
  fmt::fmt
)


add_executable(include_file_test include_file_test.cpp)
target_link_libraries(include_file_test PRIVATE taocpp::pegtl)


install(TARGETS jsonish_test toml_test config_test config_build config_reader_example include_file_test
        DESTINATION bin)
