# SPDX-FileCopyrightText: 2025 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
import sys
import najaeda
from najaeda import naja, pyslang

p = najaeda.__file__
print("Imported najaeda from:", p)

if "site-packages" not in p.replace("\\", "/"):
    raise RuntimeError(
        "najaeda was NOT imported from the installed wheel "
        f"(loaded from {p})"
    )

package_version = getattr(najaeda, "__version__", None)
if package_version is None and hasattr(najaeda, "version"):
    package_version = najaeda.version()
print("najaeda version:", package_version or "unknown")

naja_info = naja.naja_build_info()
slang_info = pyslang.slang_build_info()
if naja_info["runtime_kind"] != "shared":
    raise RuntimeError(f"unexpected Naja runtime kind: {naja_info!r}")
if slang_info["runtime_kind"] != "shared":
    raise RuntimeError(f"unexpected slang runtime kind: {slang_info!r}")

package_dir = Path(pyslang.__file__).parent
required_typing_files = (
    package_dir / "py.typed",
    package_dir / "pyslang" / "__init__.pyi",
    package_dir / "pyslang" / "ast.pyi",
)
missing_typing_files = [
    str(path) for path in required_typing_files if not path.is_file()
]
if missing_typing_files:
    raise RuntimeError(
        f"Bundled pyslang typing files are missing: {missing_typing_files}"
    )

tree = pyslang.syntax.SyntaxTree.fromText("module wheel_import_check; endmodule")
compilation = pyslang.ast.Compilation()
compilation.addSyntaxTree(tree)
root = compilation.getRoot()
if type(root).__module__ != "najaeda.pyslang.ast":
    raise RuntimeError(f"unexpected bundled pyslang type: {type(root)!r}")

print("Bundled pyslang version:", slang_info["slang_version"])
print("Basic wheel import check: OK")
