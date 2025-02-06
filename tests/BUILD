load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "config_exception_test",
    srcs = ["config_exception_test.cpp"],
    deps = [
        "@fmt",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "config_grammar_test",
    srcs = ["config_grammar_test.cpp"],
    deps = [
        "//third_party/pegtl:pegtl",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "config_helpers_test",
    srcs = ["config_helpers_test.cpp"],
    deps = [
        "@magic_enum",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "config_parser_test",
    srcs = ["config_parser_test.cpp"],
    deps = [
        "//:flexi_cfg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "math_test",
    srcs = ["math_test.cpp"],
    deps = [
        "//third_party/pegtl:pegtl",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "ordered_map_test",
    srcs = ["ordered_map_test.cpp"],
    deps = [
        "//:ordered_map",
        "@fmt",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "utils_test",
    srcs = ["utils_test.cpp"],
    deps = [
        "//:flexi_cfg",
        "@com_google_googletest//:gtest_main",
    ],
)
