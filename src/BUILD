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
