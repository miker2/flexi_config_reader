cmake_minimum_required(VERSION 3.14)

project (PARSER_LEXER)

include(FetchContent)
# Get taocpp::pegtl
FetchContent_Declare(
	taocpp_pegtl
	GIT_REPOSITORY  https://github.com/taocpp/PEGTL.git
	GIT_TAG         3.2.2
	)

FetchContent_MakeAvailable(taocpp_pegtl)


add_definitions(-DEXAMPLE_DIR="${CMAKE_SOURCE_DIR}/examples")

add_executable(jsonish_test jsonish_test.cpp)
target_link_libraries(jsonish_test PRIVATE taocpp::pegtl)


add_executable(toml_test toml_test.cpp)
target_link_libraries(toml_test PRIVATE taocpp::pegtl)

add_executable(config_test config_test.cpp)
target_link_libraries(config_test PRIVATE taocpp::pegtl)
