#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0
"""(Re)write source.json for one module version in this registry.

Usage:
  update_source.py <module> <version> <url> [<strip_prefix>]

Downloads <url>, records its integrity, and records the integrity of
every file under the version's overlay/ and patches/ directories, so
the entry stays valid after editing an overlay or patch. The layout is
the Bazel Central Registry's, so a finished entry can be copied
verbatim into a bazel-central-registry pull request.
"""

import base64
import hashlib
import json
import pathlib
import sys
import urllib.request


def integrity(data):
    return "sha256-" + base64.b64encode(hashlib.sha256(data).digest()).decode()


def main(argv):
    if len(argv) not in (4, 5):
        sys.exit(__doc__)
    module, version, url = argv[1:4]
    strip_prefix = argv[4] if len(argv) == 5 else ""
    root = pathlib.Path(__file__).resolve().parent / "modules" / module / version
    if not (root / "MODULE.bazel").is_file():
        sys.exit("{}/MODULE.bazel not found".format(root))

    with urllib.request.urlopen(url) as response:
        source = {"url": url, "integrity": integrity(response.read())}
    if strip_prefix:
        source["strip_prefix"] = strip_prefix

    for kind in ("overlay", "patches"):
        directory = root / kind
        if directory.is_dir():
            source[kind] = {
                str(path.relative_to(directory)): integrity(path.read_bytes())
                for path in sorted(directory.rglob("*"))
                if path.is_file()
            }
    if "patches" in source:
        source["patch_strip"] = 1

    (root / "source.json").write_text(json.dumps(source, indent=4) + "\n")


if __name__ == "__main__":
    main(sys.argv)
