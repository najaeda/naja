# SPDX-License-Identifier: Apache-2.0

import naja

_original_installer = naja.installLoggingHandler

def _failing_installer():
  naja.installLoggingHandler = _original_installer
  raise RuntimeError("logging failure")

naja.installLoggingHandler = _failing_installer

def constructDB(db):
  pass
