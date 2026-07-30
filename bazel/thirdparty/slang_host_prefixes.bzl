# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Computes a CMAKE_PREFIX_PATH so slang's `FetchContent_MakeAvailable`
calls (external/CMakeLists.txt) resolve fmt/tomlplusplus/boost via their
`FIND_PACKAGE_ARGS` fallback to an already-installed system copy, instead
of actually fetching over the network -- which would fail anyway, since
Bazel actions have no network access by default.

On macOS with Homebrew, packages install to a versioned Cellar path
symlinked from a stable `$(brew --prefix <pkg>)` -- not automatically on
CMake's default search path, so it must be added explicitly. On Linux,
apt-installed packages (`libfmt-dev`, etc.) already land on CMake's
default system search path, so no prefix is needed there; this rule
simply produces an empty CMAKE_PREFIX_PATH when `brew` isn't present.
"""

_PACKAGES = ["fmt", "tomlplusplus", "boost"]

def _host_prefixes_impl(repository_ctx):
    brew = repository_ctx.which("brew")
    prefixes = []
    if brew:
        for pkg in _PACKAGES:
            result = repository_ctx.execute([brew, "--prefix", pkg])
            if result.return_code == 0 and result.stdout.strip():
                prefixes.append(result.stdout.strip())

    repository_ctx.file(
        "prefixes.bzl",
        'CMAKE_PREFIX_PATH = "{}"\n'.format(";".join(prefixes)),
    )
    repository_ctx.file("BUILD.bazel", "")

host_prefixes = repository_rule(
    implementation = _host_prefixes_impl,
    local = True,
    doc = "Computes CMAKE_PREFIX_PATH for slang's find_package() fallbacks.",
)
