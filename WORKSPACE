workspace(name = "flexi_cfg")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

# PEGTL
git_repository(
    name = "taocpp_pegtl",
    remote = "https://github.com/taocpp/PEGTL.git",
    tag = "3.2.7",
)

# magic_enum
git_repository(
    name = "magic_enum",
    remote = "https://github.com/Neargye/magic_enum.git",
    tag = "v0.8.2",
)

# fmt
git_repository(
    name = "fmt",
    remote = "https://github.com/fmtlib/fmt.git",
    tag = "8.1.1",
)

# range-v3
git_repository(
    name = "range-v3",
    remote = "https://github.com/ericniebler/range-v3.git",
    tag = "0.12.0",
)

# tsl-ordered-map
# new_git_repository(
#     name = "tsl_ordered_map",
#     remote = "https://github.com/Tessil/ordered-map.git",
#     tag = "v1.1.0",
#     build_file = "@//third_party:ordered_map.BUILD",
# )

# For testing
http_archive(
    name = "com_google_googletest",
    urls = ["https://github.com/google/googletest/archive/v1.13.0.zip"],
    strip_prefix = "googletest-1.13.0",
)

# For Python bindings
http_archive(
    name = "pybind11_bazel",
    strip_prefix = "pybind11_bazel-master",
    urls = ["https://github.com/pybind/pybind11_bazel/archive/master.zip"],
)

# Add rules_foreign_cc
http_archive(
    name = "rules_foreign_cc",
    sha256 = "2a4d07cd64b0719b39a7c12218a3e507672b82a97b98c6a89d38565894cf7c51",
    strip_prefix = "rules_foreign_cc-0.9.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.9.0.tar.gz",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

# Set up rules_foreign_cc
rules_foreign_cc_dependencies() 