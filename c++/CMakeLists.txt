set(CMAKE_CXX_STANDARD 20)

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Version:  ${CMAKE_CXX_COMPILER_VERSION}")


add_executable(jsonish_test jsonish_test.cpp)
target_link_libraries(jsonish_test PRIVATE taocpp::pegtl)


add_executable(toml_test toml_test.cpp)
target_link_libraries(toml_test PRIVATE taocpp::pegtl)

add_executable(config_test config_test.cpp)
target_link_libraries(config_test PRIVATE taocpp::pegtl)

add_executable(include_file_test include_file_test.cpp)
target_link_libraries(include_file_test PRIVATE taocpp::pegtl)

install(TARGETS jsonish_test toml_test config_test include_file_test
        DESTINATION bin)
