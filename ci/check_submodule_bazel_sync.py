#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Checks that CMake's git submodule pins and Bazel's MODULE.bazel pins
for the same upstream dependency haven't silently drifted apart.

Naja is built by two independent build systems (CMake, the primary one,
and Bazel, kept as a validated smoke test via ubuntu-bazel.yml/
macos-bazel.yml). Each pins its own copy of shared dependencies --
CMake via .gitmodules/git submodule commits under thirdparty/, Bazel via
MODULE.bazel's git_override()/git_repository() commits. Nothing forces
these to move together, so bumping one without the other is an easy,
silent way for the two build systems to end up testing different
upstream code without anyone noticing.

Two dependencies (naja-if, naja-verilog) are the project's own forks
that carry a separate `bazel-support` branch (native Bazel BUILD files
added on top, not present on the branch CMake tracks) -- these can never
be an exact commit match by design, so they're checked as "the
bazel-support pin must still contain (be a descendant of, or equal to)
the submodule pin" instead, i.e. bazel-support must never fall behind
main. cpptrace and slang have no such fork and are checked for an exact
commit match. googletest is deliberately excluded: CMake pins an old
submodule dev commit while Bazel takes a BCR release (1.17.0.bcr.2) --
a different dependency-sourcing mechanism entirely, not something
meant to track in lockstep.
"""

import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MODULE_BAZEL = REPO_ROOT / "MODULE.bazel"

# path under thirdparty/ -> (bazel module/repo name, upstream remote, mode)
# mode is "exact" (commits must match) or "ancestor" (submodule commit must
# be an ancestor of, or equal to, the MODULE.bazel commit).
SYNC_SPECS = {
    "thirdparty/cpptrace": ("cpptrace", "https://github.com/jeremy-rifkin/cpptrace", "exact"),
    "thirdparty/slang": ("slang", "https://github.com/najaeda/slang", "exact"),
    "thirdparty/naja-if": ("naja-if", "https://github.com/najaeda/naja-if", "ancestor"),
    "thirdparty/naja-verilog": ("naja-verilog", "https://github.com/najaeda/naja-verilog", "ancestor"),
}


def run(args, **kwargs):
    return subprocess.run(args, check=True, capture_output=True, text=True, **kwargs)


def submodule_commit(path: str) -> str:
    # Reads the gitlink straight from the tree -- works even when the
    # submodule itself was never initialized/checked out (no
    # `submodules: true` needed in the calling workflow).
    out = run(["git", "ls-tree", "HEAD", "--", path], cwd=REPO_ROOT).stdout.strip()
    if not out:
        raise RuntimeError(f"No tree entry found for {path} (is it still a submodule?)")
    # "160000 commit <sha>\t<path>"
    return out.split()[2]


def module_bazel_commits() -> dict[str, str]:
    text = MODULE_BAZEL.read_text()
    commits = {}
    for block in re.finditer(r"git_(?:override|repository)\(([^)]*)\)", text, re.DOTALL):
        body = block.group(1)
        name_match = re.search(r'(?:module_name|name)\s*=\s*"([^"]+)"', body)
        commit_match = re.search(r'commit\s*=\s*"([^"]+)"', body)
        if name_match and commit_match:
            commits[name_match.group(1)] = commit_match.group(1)
    return commits


def is_ancestor(remote: str, older: str, newer: str) -> bool:
    if older == newer:
        return True
    scratch = REPO_ROOT / ".dependency-sync-scratch"
    scratch.mkdir(exist_ok=True)
    try:
        run(["git", "init", "-q"], cwd=scratch)
        run(["git", "remote", "add", "origin", remote], cwd=scratch)
        # No --depth here: a shallow fetch makes each commit its own
        # historyless root, so merge-base --is-ancestor can never see a
        # real ancestor relationship between them.
        run(["git", "fetch", "-q", "origin", older], cwd=scratch)
        run(["git", "fetch", "-q", "origin", newer], cwd=scratch)
        result = subprocess.run(
            ["git", "merge-base", "--is-ancestor", older, newer],
            cwd=scratch,
        )
        return result.returncode == 0
    finally:
        subprocess.run(["rm", "-rf", str(scratch)])


def main() -> int:
    bazel_commits = module_bazel_commits()
    failures = []

    for path, (bazel_name, remote, mode) in SYNC_SPECS.items():
        sub_commit = submodule_commit(path)
        bazel_commit = bazel_commits.get(bazel_name)
        if bazel_commit is None:
            failures.append(
                f"{path}: no git_override/git_repository commit found for "
                f"'{bazel_name}' in MODULE.bazel"
            )
            continue

        if mode == "exact":
            if sub_commit == bazel_commit:
                print(f"OK    {path}: submodule and MODULE.bazel both at {sub_commit}")
            else:
                failures.append(
                    f"{path}: submodule pin {sub_commit} != MODULE.bazel "
                    f"'{bazel_name}' pin {bazel_commit} (expected an exact match)"
                )
        elif mode == "ancestor":
            if is_ancestor(remote, sub_commit, bazel_commit):
                print(
                    f"OK    {path}: MODULE.bazel '{bazel_name}' pin {bazel_commit} "
                    f"contains submodule pin {sub_commit}"
                )
            else:
                failures.append(
                    f"{path}: MODULE.bazel '{bazel_name}' pin {bazel_commit} does "
                    f"NOT contain submodule pin {sub_commit} -- the bazel-support "
                    "branch has fallen behind the commit CMake tracks. Rebase/"
                    "update bazel-support and re-pin MODULE.bazel's commit."
                )
        else:
            raise AssertionError(f"unknown sync mode {mode!r}")

    print(
        "SKIP  thirdparty/googletest: CMake pins an old submodule dev commit, "
        "Bazel takes a BCR release -- different sourcing mechanisms, not "
        "checked for parity (see this script's module docstring)."
    )

    if failures:
        print("\nDependency sync check FAILED:", file=sys.stderr)
        for f in failures:
            print(f"  - {f}", file=sys.stderr)
        return 1

    print("\nAll checked dependencies are in sync.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
