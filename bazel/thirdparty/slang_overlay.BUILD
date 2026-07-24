# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Overlay BUILD file for the vendored slang repository (fetched via
# git_repository in MODULE.bazel, pinned to the commit recorded in
# internal/bazel-migration-plan.md). This becomes @slang's own root
# BUILD.bazel (via git_repository's `build_file` attribute).
#
# slang has a Python-codegen-driven CMake build (AST/syntax nodes,
# diagnostics generated via CMake custom commands) plus FetchContent for
# fmt/mimalloc/Catch2/Boost.regex/tomlplusplus -- the single highest-risk
# item in this migration (see internal/bazel-migration-plan.md, task
# #14). Rather than hand-porting all of that to native Bazel rules, this
# shells out to slang's own existing CMake build via rules_foreign_cc,
# exactly as originally scoped in the plan's "Recommended approach".
#
# Every FetchContent_Declare in external/CMakeLists.txt uses
# FIND_PACKAGE_ARGS, so FetchContent_MakeAvailable tries find_package()
# first and only actually fetches over the network on failure. On macOS
# with Homebrew this always succeeds (fmt/tomlplusplus/Boost via
# CMAKE_PREFIX_PATH -- bazel/thirdparty/slang_host_prefixes.bzl -- are
# all new enough), so the cmake() action below needs no network access at
# all there, confirmed empirically (Phase 3, task #14).
#
# Linux is a real, separate case, found while drafting Phase 8's CI
# migration (not yet confirmed against a live Ubuntu runner): slang
# requires `fmt >= 12.2` and `tomlplusplus >= 3.4`
# (external/CMakeLists.txt's FIND_PACKAGE_ARGS), but Ubuntu's own apt
# packages are older or partly missing (libfmt-dev is 8.1.1 on 22.04 /
# 9.1.0 on 24.04 -- both below 12.2; libtomlplusplus-dev only exists
# starting 24.04, at exactly 3.4.0). find_package() genuinely fails
# there, so CMake's *actual current* Ubuntu CI behavior already relies on
# FetchContent's real network fetch for fmt/tomlplusplus -- this was true
# before this migration and isn't a regression, but it does mean the
# no-network-needed property validated on macOS does not hold on Linux.
# `tags = ["requires-network"]` grants (not forces) network access to
# this action so the Linux case keeps working exactly like CMake's own
# CI does today; it's a no-op on macOS, where find_package() already
# succeeds and the action never attempts a fetch.

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
load("@slang_host_prefixes//:prefixes.bzl", "CMAKE_PREFIX_PATH")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"], exclude = [".git/**"]),
)

cmake(
    name = "slang_foreign",
    # Not add_subdirectory'd by anything -- slang's CMakeLists.txt is
    # invoked as its own top-level project, so SLANG_MASTER_PROJECT
    # auto-detects ON. Explicitly pin every option naja needs OFF back to
    # OFF (they'd otherwise default to ON as master-project), matching
    # what naja's own thirdparty/CMakeLists.txt configures when it
    # add_subdirectory()s slang instead.
    cache_entries = {
        "SLANG_INCLUDE_TOOLS": "OFF",
        "SLANG_INCLUDE_TESTS": "OFF",
        "SLANG_INCLUDE_PYLIB": "OFF",
        "SLANG_INCLUDE_DOCS": "OFF",
        "SLANG_USE_MIMALLOC": "OFF",
        "SLANG_PIC_ON": "ON",
        "CMAKE_PREFIX_PATH": CMAKE_PREFIX_PATH,
        # slang's own CMakeLists.txt unconditionally does
        # find_program(CCACHE_EXECUTABLE ccache) and, if found, sets it as
        # CMAKE_{C,CXX}_COMPILER_LAUNCHER -- on a host with a system-wide
        # ccache install, this makes the build try to write ccache's temp
        # dir under $XDG_RUNTIME_DIR (e.g. /run/user/<uid>/ccache-tmp),
        # which is outside Bazel's sandbox and read-only there, failing
        # the whole compile. Pre-seeding this cache variable to the
        # standard CMake "not found" sentinel makes find_program() treat
        # it as already-searched and skip re-detecting ccache, so
        # find_package()'s own result (real or not) is never overridden by
        # a nonexistent one -- same idiom as any other pre-seeded
        # find_program/find_package cache variable.
        "CCACHE_EXECUTABLE": "CCACHE_EXECUTABLE-NOTFOUND",
    },
    # Belt-and-suspenders for the same ccache issue: if CCACHE_EXECUTABLE
    # somehow still resolves (e.g. a future slang version searches
    # differently), CCACHE_DISABLE=1 makes ccache itself a transparent
    # passthrough to the real compiler -- no cache, no temp dir, so
    # nothing to fail to create.
    env = {
        "CCACHE_DISABLE": "1",
    },
    lib_source = ":all_srcs",
    out_static_libs = ["libsvlang.a"],
    tags = ["requires-network"],
)

# external/CMakeLists.txt only `install()`s external/boost_unordered.hpp
# and external/boost_concurrent.hpp `if(NOT Boost_FOUND)` -- but
# include/slang/util/{FlatMap,ConcurrentMap}.h (both public, installed
# headers) #include them *unconditionally* regardless of whether real
# Boost was found. Since our build finds a real system Boost (>=1.87) via
# CMAKE_PREFIX_PATH, `cmake()`'s installed include/ tree above is missing
# both files -- a real gap in slang's own CMake packaging, only exposed by
# consuming it through a real install tree (add_subdirectory consumers,
# like naja's own CMake build today, never hit this: their build-interface
# include path always covers external/ regardless of the install()
# conditional). Rather than patching slang's CMakeLists.txt, supplement
# with these two headers directly from source.
cc_library(
    name = "slang_boost_shims",
    hdrs = [
        "external/boost_unordered.hpp",
        "external/boost_concurrent.hpp",
    ],
    includes = ["external"],
)

cc_library(
    name = "slang",
    deps = [
        ":slang_boost_shims",
        ":slang_foreign",
        "@fmt//:fmt",
        "@tomlplusplus//:tomlplusplus",
    ],
    visibility = ["//visibility:public"],
)
