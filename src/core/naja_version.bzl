# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Git hash compiled into NajaVersion.h (CMake runs `git log` instead)."""

def naja_git_hash():
    """Returns the commit a registry version of naja was built from.

    Registry entries for unreleased commits use BCR's
    `<release>-<YYYYMMDD>-<commit>` version scheme, so the hash is the
    last component. Anything else (a release, or naja as the root
    module) reports "unknown". Must be called from a BUILD file.
    """
    parts = native.module_version().split("-")
    if len(parts) == 3:
        return parts[2]
    return "unknown"
