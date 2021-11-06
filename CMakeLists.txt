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



add_executable(maps_test maps_test.cpp)

target_link_libraries(maps_test PRIVATE taocpp::pegtl)
