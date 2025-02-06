load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "flexi_cfg",
    srcs = [
        "src/config_helpers.cpp",
        "src/config_parser.cpp",
        "src/config_reader.cpp",
        "src/math_helpers.cpp",
    ],
    hdrs = glob([
        "include/flexi_cfg/**/*.h",
        "include/details/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//third_party/pegtl:pegtl",
        "@fmt",
        "@range-v3",
        # "@tsl_ordered_map",
        "@magic_enum",
    ],
    defines = [
        'EXAMPLE_DIR=\\"%s/examples\\"' % package_name(),
    ],
)

cc_library(
    name = "ordered_map",
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)