# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Configuration regressions without network access or initialized submodules."""

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import check_submodule_bazel_sync as check


class DependencyConfigurationTest(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.rc = self.root / ".bazelrc"
        self.rc.write_text(
            "common --override_module=naja-if=%workspace%/thirdparty/naja-if\n"
            "common:registered --override_module=naja-if=\n")
        with self.rc.open("a") as rc:
            rc.write("common --override_module=naja-verilog=%workspace%/thirdparty/naja-verilog\n"
                     "common:registered --override_module=naja-verilog=\n")
        parser = self.root / "thirdparty/naja-verilog"
        parser.mkdir(parents=True)
        (parser / "MODULE.bazel").write_text('module(name = "naja-verilog")\n')
        (parser / "BUILD.bazel").write_text("")
        self.module = self.root / "thirdparty/naja-if"
        self.module.mkdir(parents=True)
        (self.module / "MODULE.bazel").write_text('module(name = "naja-if")\n')
        (self.module / "BUILD.bazel").write_text("")

    def test_source_checkout_needs_no_registry_release(self):
        self.assertEqual([], check.check_configurations(self.root))

    def test_wrong_checkout_is_rejected(self):
        self.rc.write_text(self.rc.read_text().replace("%workspace%/thirdparty/naja-if", "/tmp/unrelated"))
        self.assertTrue(any("default Bazel" in error for error in check.check_configurations(self.root)))

    def test_registered_must_clear_override(self):
        self.rc.write_text(self.rc.read_text().replace("common:registered", "common:other"))
        self.assertTrue(any("clear" in error for error in check.check_configurations(self.root)))

    def test_uninitialized_checkout_has_actionable_error(self):
        (self.module / "MODULE.bazel").unlink()
        self.assertTrue(any("git submodule update" in error for error in check.check_configurations(self.root)))

    def test_wrong_module_is_rejected(self):
        (self.module / "MODULE.bazel").write_text('module(name = "wrong")\n')
        self.assertTrue(any("declare module" in error for error in check.check_configurations(self.root)))

    def test_registered_does_not_require_checkout(self):
        (self.module / "MODULE.bazel").unlink()
        module = self.root / "MODULE.bazel"
        module.write_text('bazel_dep(name = "naja-if", version = "older-release")\n'
                          'bazel_dep(name = "naja-verilog", version = "older-release")\n')
        with patch.object(check, "BAZELRC", self.rc), patch.object(check, "MODULE_BAZEL", module):
            self.assertEqual(0, check.main(["--registered"]))


if __name__ == "__main__":
    unittest.main()
