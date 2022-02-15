cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

project (PARSER_LEXER)


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


add_definitions(-DEXAMPLE_DIR="${CMAKE_SOURCE_DIR}/examples")

add_subdirectory(c++)
