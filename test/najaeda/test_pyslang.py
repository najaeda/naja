# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import ast
import ctypes
import importlib.util
import os
from pathlib import Path
import subprocess
import sys

import pytest


def _bundled_pyslang():
    try:
        from najaeda import pyslang
    except ImportError:
        pytest.skip("bundled pyslang is available in BUILD_NAJA_PYTHON builds")
    return pyslang


def test_bundled_pyslang_smoke():
    pyslang = _bundled_pyslang()

    info = pyslang.slang_build_info()
    assert info["provider"] == "najaeda.pyslang"
    assert info["api_version"] == 1
    assert info["runtime_kind"] == "shared"
    assert info["git_hash"]
    assert info["build_id"].startswith("naja-slang-native-v1:")

    capsule_is_valid = ctypes.pythonapi.PyCapsule_IsValid
    capsule_is_valid.argtypes = [ctypes.py_object, ctypes.c_char_p]
    capsule_is_valid.restype = ctypes.c_int
    assert capsule_is_valid(pyslang._C_API, b"najaeda.pyslang._C_API") == 1

    package_dir = Path(pyslang.__file__).parent
    assert (package_dir / "py.typed").is_file()
    stub_dir = package_dir / "pyslang"
    root_stub = stub_dir / "__init__.pyi"
    assert root_stub.is_file()
    assert (stub_dir / "ast.pyi").is_file()
    for stub in stub_dir.glob("*.pyi"):
        ast.parse(stub.read_text(encoding="utf-8"), filename=str(stub))
    assert "capsule object" not in root_stub.read_text(encoding="utf-8")

    tree = pyslang.syntax.SyntaxTree.fromText("module bundled; endmodule")
    compilation = pyslang.ast.Compilation()
    compilation.addSyntaxTree(tree)
    root = compilation.getRoot()
    assert type(root).__module__ == "najaeda.pyslang.ast"


@pytest.mark.skipif(
    importlib.util.find_spec("pyslang") is None,
    reason="external top-level pyslang is not installed",
)
@pytest.mark.parametrize("external_first", [True, False])
def test_external_pyslang_coexists_in_both_import_orders(external_first):
    first, second = (
        ("import pyslang as external", "from najaeda import pyslang as bundled")
        if external_first
        else ("from najaeda import pyslang as bundled", "import pyslang as external")
    )
    script = f"""
{first}
{second}
external_tree = external.syntax.SyntaxTree.fromText('module external; endmodule')
bundled_tree = bundled.syntax.SyntaxTree.fromText('module bundled; endmodule')
external_compilation = external.ast.Compilation()
bundled_compilation = bundled.ast.Compilation()
external_compilation.addSyntaxTree(external_tree)
bundled_compilation.addSyntaxTree(bundled_tree)
external_root = external_compilation.getRoot()
bundled_root = bundled_compilation.getRoot()
assert type(external_root) is not type(bundled_root)
assert type(external_root).__module__.startswith('pyslang.')
assert type(bundled_root).__module__ == 'najaeda.pyslang.ast'
"""
    subprocess.run(
        [sys.executable, "-c", script],
        check=True,
        env=os.environ.copy(),
    )
