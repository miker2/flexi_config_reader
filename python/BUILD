load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")
load("@rules_python//python:defs.bzl", "py_test")

pybind_extension(
    name = "flexi_cfg",
    srcs = ["flexi_cfg_py.cpp"],
    deps = ["//src:flexi_cfg"],
)

py_test(
    name = "test_flexi_cfg",
    srcs = ["test_flexi_cfg.py"],
    imports = ["."],
    data = ["//examples:test_configs"],
    deps = [":flexi_cfg"],
    main = "test_flexi_cfg.py",
)
