load("@rules_cc//cc:defs.bzl", "cc_test")

# Tests that reach for files under examples/. They locate them through
# EXAMPLE_DIR, which is relative to the runfiles root under Bazel.
# The value is the test's extra environment; examples/config_example13.cfg
# resolves its include through ${TEST_DIR}.
_CONFIG_TESTS = {
    "config_exception_test": {},
    "config_grammar_test": {},
    "config_parse_test": {"TEST_DIR": "env"},
}

# Tests that only exercise self-contained code.
_UNIT_TESTS = [
    "config_helpers_test",
    "math_test",
    "ordered_map_test",
    "utils_test",
]

[
    cc_test(
        name = name,
        srcs = [name + ".cpp"],
        data = ["//examples:test_configs"],
        defines = ['EXAMPLE_DIR=\\"examples\\"'],
        env = env,
        deps = [
            "//src:flexi_cfg",
            "@com_google_googletest//:gtest_main",
        ],
    )
    for name, env in _CONFIG_TESTS.items()
]

[
    cc_test(
        name = name,
        srcs = [name + ".cpp"],
        deps = [
            "//src:flexi_cfg",
            "@com_google_googletest//:gtest_main",
        ],
    )
    for name in _UNIT_TESTS
]
