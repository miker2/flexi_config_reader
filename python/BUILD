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
    data = ["//examples:test_configs"],
    imports = ["."],
    main = "test_flexi_cfg.py",
    # A sanitized extension module needs the ASan runtime preloaded into the
    # interpreter, which is a separate exercise; --config=sanitize skips it.
    tags = ["no_sanitize"],
    deps = [":flexi_cfg"],
)
