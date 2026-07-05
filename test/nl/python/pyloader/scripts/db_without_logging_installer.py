# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import sys
import naja

_original_installer = naja.installLoggingHandler
del naja.installLoggingHandler

def __getattr__(name):
  if name == "installLoggingHandler":
    naja.installLoggingHandler = _original_installer
    del naja.__getattr__
    raise AttributeError(name)
  raise AttributeError(name)

naja.__getattr__ = __getattr__

def constructDB(db):
  pass
