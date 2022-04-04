include(CTest)
include(GoogleTest)

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
add_test(NAME UtilsTest COMMAND utils_test)

gtest_discover_tests(utils_test)
