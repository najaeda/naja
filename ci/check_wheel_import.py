# SPDX-FileCopyrightText: 2025 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import sys
import najaeda

p = najaeda.__file__
print("Imported najaeda from:", p)

if "site-packages" not in p.replace("\\", "/"):
    raise RuntimeError(
        "najaeda was NOT imported from the installed wheel "
        f"(loaded from {p})"
    )

print("najaeda version:", getattr(najaeda, "__version__", "unknown"))
print("Basic wheel import check: OK")