# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import builtins
import sys
import naja

builtins._saved_naja_module = naja
del sys.modules["naja"]

def constructDB(db):
  pass
