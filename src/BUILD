load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

cc_library(
    name = "flexi_cfg",
    srcs = [
        "config_helpers.cpp",
        "config_parser.cpp",
        "config_reader.cpp",
        "math_helpers.cpp",
    ],
    # range-v3's meta.hpp forward-declares standard containers itself whenever
    # Apple clang is detected. Those declarations do not compile against recent
    # libc++, which dropped `_LIBCPP_TEMPLATE_VIS`. Opt out and let it include
    # the real headers instead. No-op for the toolchains that skip that block.
    defines = ["META_NO_STD_FORWARD_DECLARATIONS"],
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

# Exercises the include-filename grammar. Reports failure through its exit
# status, so it works as a test target directly.
cc_test(
    name = "include_file_test",
    srcs = ["include_file_test.cpp"],
    visibility = ["//visibility:public"],
    deps = [":flexi_cfg"],
)

sh_test(
    name = "config_build_examples_test",
    srcs = ["config_build_examples_test.sh"],
    args = ["$(rootpath :config_build)"],
    data = [
        ":config_build",
        "//examples:test_configs",
    ],
    # examples/config_example13.cfg resolves its include through ${TEST_DIR}.
    env = {"TEST_DIR": "env"},
)
