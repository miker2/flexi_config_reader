load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "flexi_cfg",
    srcs = [
        "config_helpers.cpp",
        "config_parser.cpp",
        "config_reader.cpp",
        "math_helpers.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//include:flexi_cfg_hdrs",
        "@pegtl",
        "@magic_enum",
        "@fmt",
        "@range-v3",
    ],
)

cc_binary(
    name = "config_build",
    srcs = ["config_build.cpp"],
    deps = [":flexi_cfg"],
    data = ["//examples:test_configs"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "config_reader_example",
    srcs = ["config_reader_example.cpp"],
    deps = [":flexi_cfg"],
    visibility = ["//visibility:public"],
    defines = ["EXAMPLE_DIR=\\\"examples\\\""],
    data = ["//examples:test_configs"],
)

cc_binary(
    name = "include_file_test",
    srcs = ["include_file_test.cpp"],
    deps = [":flexi_cfg"],
    visibility = ["//visibility:public"],
)
