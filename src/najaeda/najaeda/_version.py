# SPDX-FileCopyrightText: 2025 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import importlib

try:
    naja = importlib.import_module("naja")
except ModuleNotFoundError:
    naja = importlib.import_module(".naja", __package__)


def version():
    """Get the version of Naja."""
    return naja.getVersion()


def git_hash():
    """Get the git hash of Naja."""
    return naja.getGitHash()
