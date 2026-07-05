# SPDX-License-Identifier: Apache-2.0

import builtins
import sys
import naja

builtins._saved_naja_module = naja
del sys.modules["naja"]

def constructDB(db):
  pass
