# SPDX-FileCopyrightText: 2025 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import importlib

naja = importlib.import_module("naja")

from ._version import version, git_hash

__all__ = ["naja", "version", "git_hash"]
