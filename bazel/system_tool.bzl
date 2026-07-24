# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Repository rule locating a system-installed tool via PATH.

Used for `bison`/`m4`, the same tools CMake's `find_package(BISON
REQUIRED)` (thirdparty/lefdef/CMakeLists.txt) resolves against. Mirrors
the pragmatic system-wrapping pattern this migration already uses for
TBB/Boost (bazel/thirdparty/{tbb,boost}/repo.bzl) and for naja-verilog's
own Bazel module (see internal/bazel-migration-plan.md) rather than
vendoring/building these toolchains from source under Bazel.
"""

def _system_tool_repository_impl(repository_ctx):
    tool = repository_ctx.attr.tool
    found = repository_ctx.which(tool)
    if not found:
        fail(("{} not found on PATH; required to build naja's vendored " +
              "lefdef parser. Is it installed? (brew install {} / " +
              "apt install {})").format(tool, tool, tool))

    repository_ctx.symlink(found, tool)
    repository_ctx.file(
        "BUILD.bazel",
        'exports_files(["{}"])\n'.format(tool),
    )

system_tool_repository = repository_rule(
    implementation = _system_tool_repository_impl,
    attrs = {
        "tool": attr.string(mandatory = True, doc = "Tool name to locate on PATH."),
    },
    local = True,
    doc = "Locates a system-installed tool (bison/m4) via PATH.",
)
