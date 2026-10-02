#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Checks that CMake's git submodule pins and Bazel's pins for the same
upstream dependency haven't silently drifted apart.

Naja is built by two independent build systems (CMake, the primary one,
and Bazel, kept as a validated smoke test via ubuntu-bazel.yml/
macos-bazel.yml). Each pins its own copy of shared dependencies --
CMake via .gitmodules/git submodule commits under thirdparty/, Bazel via
the commit archive that MODULE.bazel's bazel_dep version resolves to in
the in-tree registry (bazel/registry/modules/<name>/<version>/source.json). Nothing forces
these to move together, so bumping one without the other is an easy,
silent way for the two build systems to end up testing different
upstream code without anyone noticing.

naja-if is the project's own fork that still carries a separate
`bazel-support` branch (native Bazel BUILD files added on top, not
present on the branch CMake tracks) -- it can never be an exact commit
match by design, so it's checked as "the bazel-support pin must still
contain (be a descendant of, or equal to) the submodule pin" instead,
i.e. bazel-support must never fall behind main. naja-verilog's own
`bazel-support` branch was merged into `main` (2026), so it's now
checked for an exact match like slang, both tracking one branch instead
of two. cpptrace is not a Bazel dependency at all (naja's use of it is
commented out), so there is nothing to compare. googletest is deliberately excluded: CMake
pins an old submodule dev commit while Bazel takes a BCR release
(1.17.0.bcr.2) -- a different dependency-sourcing mechanism entirely,
not something meant to track in lockstep.
"""

import json
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MODULE_BAZEL = REPO_ROOT / "MODULE.bazel"
REGISTRY_MODULES = REPO_ROOT / "bazel" / "registry" / "modules"

# path under thirdparty/ -> (bazel module name, upstream remote, mode)
# mode is "exact" (commits must match) or "ancestor" (submodule commit must
# be an ancestor of, or equal to, the Bazel commit).
SYNC_SPECS = {
    "thirdparty/slang": ("sv-lang", "https://github.com/MikePopoloski/slang", "exact"),
    "thirdparty/naja-if": ("naja-if", "https://github.com/najaeda/naja-if", "ancestor"),
    "thirdparty/naja-verilog": ("naja-verilog", "https://github.com/najaeda/naja-verilog", "exact"),
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
    for name, version in re.findall(
        r'bazel_dep\(name = "([^"]+)", version = "([^"]+)"', text
    ):
        source = REGISTRY_MODULES / name / version / "source.json"
        if not source.is_file():
            continue  # served by BCR, not pinned to a commit here
        url = json.loads(source.read_text())["url"]
        commit_match = re.search(r"/archive/([0-9a-f]{40})\.tar\.gz$", url)
        if commit_match:
            commits[name] = commit_match.group(1)
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
                f"{path}: no commit archive found for '{bazel_name}' in "
                "bazel/registry for the version MODULE.bazel depends on"
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
                    "update bazel-support and add a registry version for it."
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
