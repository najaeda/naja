# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Repository rule computing the current git commit hash, for generating
src/core/NajaVersion.h (mirrors CMakeLists.txt's `configure_file` step,
which substitutes `@NAJA_GIT_HASH@` with `git log -1 --pretty=format:%h`
run at CMake-configure time -- src/core/CMakeLists.txt).

This value matters beyond a diagnostic string: per the NajaIF snapshot
compatibility work (see CLAUDE.md), `naja::NAJA_GIT_HASH` is one of the
two producer-identity fields `SNLCapnP::load()` checks for an exact match
before deserializing a snapshot.

Caveat: unlike CMake (which reruns `git log` on every configure), this
value is captured once, when Bazel evaluates this repository rule, and
can go stale relative to the actually-checked-out commit until something
forces Bazel to re-evaluate it (e.g. a clean build, or `bazel sync`).
Bazel's proper version-stamping mechanism (`ctx.info_file`/`--stamp`) only
applies to executable targets (`cc_binary`/`cc_test`), not the `cc_library`
that needs this header compiled in -- getting genuinely fresh-per-build
stamping into a library would need a custom rule reading `.git/HEAD` and
`.git/refs/**` directly as real (unsandboxed) build inputs, which is a
larger undertaking deferred for now. Flagged here rather than silently
accepted.
"""

def _git_version_impl(repository_ctx):
    workspace_root = repository_ctx.workspace_root
    git = repository_ctx.which("git")

    git_hash = "unknown"
    if git:
        result = repository_ctx.execute(
            [git, "log", "-1", "--pretty=format:%h"],
            working_directory = str(workspace_root),
        )
        if result.return_code == 0 and result.stdout.strip():
            git_hash = result.stdout.strip()

    repository_ctx.file(
        "git_version.bzl",
        'NAJA_GIT_HASH = "{}"\n'.format(git_hash),
    )
    repository_ctx.file("BUILD.bazel", "")

git_version = repository_rule(
    implementation = _git_version_impl,
    local = True,
    doc = "Computes the current git commit hash for NajaVersion.h.",
)
