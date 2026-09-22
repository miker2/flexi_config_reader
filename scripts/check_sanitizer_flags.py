#!/usr/bin/env python3
"""Fail if CMake's CFG_SANITIZE flags and Bazel's --config=sanitize flags differ.

Both build systems have to declare the sanitizer compiler flags in their own
syntax, so they are written down twice. This is the check that keeps the two
copies identical. Run it from the repository root; CI runs it before the
sanitized builds.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def cmake_flags() -> set[str]:
    text = (ROOT / "CMakeLists.txt").read_text()
    m = re.search(r"set\(_cfg_sanitize_flags\s+(.*?)\)", text, re.S)
    if not m:
        sys.exit("CMakeLists.txt: could not find set(_cfg_sanitize_flags ...)")
    return {tok for tok in m.group(1).split() if tok.startswith("-")}


def bazel_flags() -> set[str]:
    text = (ROOT / ".bazelrc").read_text()
    flags = set(re.findall(r"^build:sanitize\s+--copt=(\S+)", text, re.M))
    if not flags:
        sys.exit(".bazelrc: could not find any 'build:sanitize --copt=' lines")
    return flags


def main() -> int:
    cmake, bazel = cmake_flags(), bazel_flags()
    if cmake == bazel:
        print("sanitizer flags match:", " ".join(sorted(cmake)))
        return 0
    print("sanitizer flag lists have drifted:", file=sys.stderr)
    for f in sorted(cmake - bazel):
        print(f"  only in CMakeLists.txt: {f}", file=sys.stderr)
    for f in sorted(bazel - cmake):
        print(f"  only in .bazelrc:       {f}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
