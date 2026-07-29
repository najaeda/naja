# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Overlay BUILD file for the vendored fmt repository (fetched via
# git_repository in MODULE.bazel, pinned to the exact 12.2.0 tag commit
# -- matching thirdparty/slang/external/CMakeLists.txt's own
# `GIT_TAG 12.2.0` / `FIND_PACKAGE_ARGS 12.2` requirement exactly).
#
# Supersedes the earlier pkg-config-based bazel/thirdparty/fmt/repo.bzl
# after live Ubuntu CI surfaced a real ABI mismatch: Ubuntu's apt fmt
# (libfmt-dev is v8/v9, both way below 12.2) fails slang's own internal
# find_package(fmt 12.2) version check, so slang's own build falls
# through to *its* FetchContent network fetch and links against a real
# fmt 12.2.0 -- meaning libsvlang.a ends up referencing `fmt::v12::...`
# symbols that the old apt-pkg-config-wrapped @fmt (v8/v9's `fmt::v8`/
# `fmt::v9` namespace) can never satisfy (undefined reference errors,
# confirmed on live CI: `fmt::v12::detail::allocate`,
# `fmt::v12::detail::print`, etc.). Same root cause and same fix as
# bazel/thirdparty/tomlplusplus_overlay.BUILD's version mismatch: pin our
# own copy to the exact version slang itself requires, everywhere,
# instead of depending on whatever a given platform's package manager
# happens to ship.
#
# Mirrors fmt's own CMakeLists.txt: `add_library(fmt src/format.cc ...)`
# plus `src/os.cc` (FMT_OS, default ON -- slang's OS.cpp needs its
# `fmt::v12::detail::print(FILE*, ...)` overload).

cc_library(
    name = "fmt",
    srcs = [
        "src/format.cc",
        "src/os.cc",
    ],
    hdrs = glob(["include/fmt/**"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)
