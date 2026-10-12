#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Validate shared source revisions and the source/registered configuration split.

naja-if and naja-verilog use CMake's checkouts in default Bazel builds. Their
registered versions advance independently; --registered checks configuration only, while
Bazel builds/tests establish registry compatibility. Slang
still relies on registry build adaptations, so its source revision must match
CMake until native upstream module support permits direct checkout selection.
Googletest deliberately uses different sourcing mechanisms.
"""

import argparse
import json
import re
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MODULE_BAZEL = REPO_ROOT / "MODULE.bazel"
BAZELRC = REPO_ROOT / ".bazelrc"

# Transitional registry-backed source dependencies; the others are checked locally.
SYNC_SPECS = {
    "thirdparty/slang": "sv-lang",
}

LOCAL_MODULES = {"naja-if": "thirdparty/naja-if", "naja-verilog": "thirdparty/naja-verilog"}


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


def registries() -> list[str]:
    """The --registry URLs from .bazelrc, in lookup order."""
    return re.findall(r"^common --registry=(\S+?)/?$", BAZELRC.read_text(), re.M)


def registry_source_json(name: str, version: str) -> dict | None:
    """source.json for name@version from the first registry that has it,
    as Bazel resolves it."""
    for registry in registries():
        url = f"{registry}/modules/{name}/{version}/source.json"
        try:
            with urllib.request.urlopen(url) as response:
                return json.load(response)
        except urllib.error.HTTPError as e:
            if e.code != 404:
                raise
    return None


def tag_commit(remote: str, tag: str) -> str:
    """The commit a release tag (as in a BCR release's archive URL) points to."""
    out = run(["git", "ls-remote", remote, f"refs/tags/{tag}", f"refs/tags/{tag}^{{}}"]).stdout
    refs = dict(reversed(line.split("\t")) for line in out.splitlines())
    # An annotated tag's ^{} entry is the commit; a lightweight tag has none.
    return refs.get(f"refs/tags/{tag}^{{}}") or refs[f"refs/tags/{tag}"]


def module_bazel_commits() -> dict[str, str]:
    text = MODULE_BAZEL.read_text()
    commits = {}
    for name, version in re.findall(
        r'bazel_dep\(name = "([^"]+)", version = "([^"]+)"', text
    ):
        if name not in SYNC_SPECS.values():
            continue
        source = registry_source_json(name, version)
        if source is None:
            continue
        url = source["url"]
        commit_match = re.search(r"/archive/([0-9a-f]{40})\.tar\.gz$", url)
        tag_match = re.search(
            r"^(https://github\.com/[^/]+/[^/]+)/archive/refs/tags/(.+)\.tar\.gz$", url
        )
        if commit_match:
            commits[name] = commit_match.group(1)
        elif tag_match:
            commits[name] = tag_commit(*tag_match.groups())
    return commits


def check_configurations(root: Path = REPO_ROOT) -> list[str]:
    failures = []
    rc = (root / ".bazelrc").read_text()
    for name, path in LOCAL_MODULES.items():
        overrides = re.findall(
            rf"^common --override_module={re.escape(name)}=(.*)$", rc, re.M
        )
        if overrides != [f"%workspace%/{path}"]:
            failures.append(f"{name}: default Bazel must select %workspace%/{path}")
        registered = re.findall(
            rf"^common:registered --override_module={re.escape(name)}=(.*)$", rc, re.M
        )
        if registered != [""]:
            failures.append(f"{name}: registered configuration must clear its local override")
        module = root / path / "MODULE.bazel"
        if not module.is_file():
            failures.append(f"{path}: initialize the submodule with git submodule update --init {path}")
        elif not re.search(rf'\bname\s*=\s*"{re.escape(name)}"', module.read_text()):
            failures.append(f"{path}: MODULE.bazel must declare module {name}")
        if not (root / path / "BUILD.bazel").is_file():
            failures.append(f"{path}: native BUILD.bazel is missing")
    return failures


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--registered", action="store_true",
                        help="check registered configuration without requiring local submodules")
    args = parser.parse_args(argv)
    if args.registered:
        failures = []
        rc = BAZELRC.read_text()
        module_text = MODULE_BAZEL.read_text()
        for name in LOCAL_MODULES:
            clears = re.findall(rf"^common:registered --override_module={re.escape(name)}=(.*)$", rc, re.M)
            if clears != [""]:
                failures.append(f"{name}: registered configuration must clear its local override")
            if not re.search(rf'bazel_dep\(name = "{re.escape(name)}", version = "[^"]+"', module_text):
                failures.append(f"{name}: missing registered dependency version")
        if not failures:
            print("Registered configuration selects versioned dependencies; run Bazel build/test to validate compatibility.")
    else:
        failures = check_configurations()
        for name, path in LOCAL_MODULES.items():
            if not failures:
                print(f"OK    {name}: CMake and default Bazel use {path}; registry publication is independent")
        bazel_commits = module_bazel_commits()
        for path, name in SYNC_SPECS.items():
            source_commit = submodule_commit(path)
            registered_commit = bazel_commits.get(name)
            if source_commit != registered_commit:
                failures.append(f"{path}: registry-backed source revision {registered_commit} != CMake {source_commit}; native upstream Bazel support is needed to use an unpublished checkout")
            else:
                print(f"OK    {path}: source revisions match at {source_commit}")
    if failures:
        print("Dependency configuration check FAILED:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
