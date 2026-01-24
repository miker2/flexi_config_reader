load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
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
        "@fmt",
        "@magic_enum",
        "@pegtl",
        "@range-v3",
    ],
)

cc_binary(
    name = "config_build",
    srcs = ["config_build.cpp"],
    data = ["//examples:test_configs"],
    visibility = ["//visibility:public"],
    deps = [":flexi_cfg"],
)

cc_binary(
    name = "config_reader_example",
    srcs = ["config_reader_example.cpp"],
    data = ["//examples:test_configs"],
    defines = ["EXAMPLE_DIR=\\\"examples\\\""],
    visibility = ["//visibility:public"],
    deps = [":flexi_cfg"],
)

cc_binary(
    name = "include_file_test",
    srcs = ["include_file_test.cpp"],
    visibility = ["//visibility:public"],
    deps = [":flexi_cfg"],
)
