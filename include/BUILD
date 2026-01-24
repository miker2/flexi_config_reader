load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "flexi_cfg_hdrs",
    hdrs = glob(["flexi_cfg/**/*.h"]),
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "@fmt",
        "@magic_enum",
    ],
)
