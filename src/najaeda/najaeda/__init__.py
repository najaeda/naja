# SPDX-FileCopyrightText: 2025 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import importlib

try:
    naja = importlib.import_module(".naja", __name__)
except ModuleNotFoundError as error:
    if error.name != f"{__name__}.naja":
        raise
    # Some development layouts expose the extension directly on PYTHONPATH.
    naja = importlib.import_module("naja")

from ._version import version, git_hash
from ._logging import configure_native_logging

__version__ = version()

configure_native_logging()

__all__ = ["naja", "version", "git_hash", "__version__", "configure_native_logging"]
