# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Repository rule exposing the absolute workspace root path.

Needed only for tests that change the process's current working
directory mid-test (e.g. testing "resolve this path relative to CWD"
behavior) and therefore can't rely on a workspace-relative `-D` path
define the way most other ported tests do (see e.g.
test/nl/formats/verilog/frontend/BUILD.bazel) -- once CWD moves, a
relative define no longer resolves. CMake sidesteps this for free since
CMAKE_CURRENT_SOURCE_DIR is already absolute; this replicates that exact
same non-hermetic (build-machine-specific path baked at build time)
behavior deliberately, rather than leaving such tests broken.

Currently only test/nl/formats/systemverilog/frontend/BUILD.bazel (for
SNLSVConstructorTestSimple.cpp's
parseSimpleModuleUsesDefaultDiagnosticsReportPath, the one test in the
whole ported suite that calls std::filesystem::current_path() to change
CWD) needs this.
"""

def _workspace_root_impl(repository_ctx):
    repository_ctx.file(
        "workspace_root.bzl",
        'WORKSPACE_ROOT = "{}"\n'.format(repository_ctx.workspace_root),
    )
    repository_ctx.file("BUILD.bazel", "")

workspace_root = repository_rule(
    implementation = _workspace_root_impl,
    local = True,
    doc = "Exposes the absolute workspace root path as a Starlark constant.",
)
