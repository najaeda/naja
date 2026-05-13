# SPDX-FileCopyrightText: 2025 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import importlib

try:
    naja = importlib.import_module("naja")
except ModuleNotFoundError:
    # Test and bundled package layouts place the extension module inside the
    # najaeda package rather than on PYTHONPATH as a top-level module.
    naja = importlib.import_module(".naja", __name__)

from ._version import version, git_hash

__all__ = ["naja", "version", "git_hash"]
