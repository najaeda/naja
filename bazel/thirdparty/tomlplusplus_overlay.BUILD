# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Overlay BUILD file for the vendored tomlplusplus repository (fetched
# via git_repository in MODULE.bazel, pinned to the v3.4.0 tag's commit
# -- matching the exact version thirdparty/slang/external/CMakeLists.txt
# requires: `FIND_PACKAGE_ARGS 3.4`).
#
# Superseded the earlier pkg-config-based bazel/thirdparty/tomlplusplus/repo.bzl
# after live Ubuntu CI showed `libtomlplusplus-dev` isn't installable via
# apt there at all (only ships from Ubuntu 24.04, and even then exactly
# at 3.4.0 -- no margin) -- the same brittle-host-package trap this
# migration already avoided for naja-if/naja-verilog/slang by vendoring
# a pinned fetch instead. This is that same fix applied here: no system
# package dependency on any platform.
#
# toml++ is normally header-only (TOML_HEADER_ONLY defaults to 1), but
# @slang//:slang links a *compiled* tomlplusplus (found via Homebrew on
# macOS, itself built non-header-only) -- its own object files reference
# out-of-line symbols like `toml::v3::table::table()` as undefined,
# discovered via a real link error (Phase 6/7). Matching that: this
# vendors the tiny upstream-provided single-TU implementation file
# (src/toml.cpp, self-contained -- it `#define`s TOML_IMPLEMENTATION and
# TOML_HEADER_ONLY 0 itself if not already set) as a real compiled
# cc_library, so `-ltomlplusplus`-equivalent symbols exist regardless of
# what any given platform's system package manager ships.

cc_library(
    name = "tomlplusplus",
    srcs = ["src/toml.cpp"],
    hdrs = glob(["include/toml++/**"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)
