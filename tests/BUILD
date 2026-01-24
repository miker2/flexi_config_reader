load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "config_exception_test",
    srcs = ["config_exception_test.cpp"],
    defines = ["EXAMPLE_DIR=\\\"examples\\\""],
    data = ["//examples:test_configs"],
    deps = [
        "//src:flexi_cfg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "config_grammar_test",
    srcs = ["config_grammar_test.cpp"],
    defines = ["EXAMPLE_DIR=\\\"examples\\\""],
    data = ["//examples:test_configs"],
    deps = [
        "//src:flexi_cfg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "config_helpers_test",
    srcs = ["config_helpers_test.cpp"],
    defines = ["EXAMPLE_DIR=\\\"examples\\\""],
    deps = [
        "//src:flexi_cfg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "config_parse_test",
    srcs = ["config_parse_test.cpp"],
    defines = [
        "EXAMPLE_DIR=\\\"examples\\\"",
    ],
    data = ["//examples:test_configs"],
    deps = [
        "//src:flexi_cfg",
        "@com_google_googletest//:gtest_main",
    ],
    env = {
        "TEST_DIR" : "env",
    },
)

cc_test(
    name = "math_test",
    srcs = ["math_test.cpp"],
    defines = ["EXAMPLE_DIR=\\\"examples\\\""],
    deps = [
        "//src:flexi_cfg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "utils_test",
    srcs = ["utils_test.cpp"],
    defines = ["EXAMPLE_DIR=\\\"examples\\\""],
    deps = [
        "//src:flexi_cfg",
        "@com_google_googletest//:gtest_main",
    ],
)



