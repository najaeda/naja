# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from najaeda import naja

def version():
    """Get the version of Naja."""
    return naja.getVersion()

def git_hash():
    """Get the git hash of Naja."""
    return naja.getGitHash()