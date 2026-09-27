# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import importlib
import sys
import unittest
from unittest.mock import patch

import najaeda


class NajaedaSchematicTest(unittest.TestCase):
    def setUp(self):
        self.previous_module = sys.modules.pop("najaeda.schematic", None)
        self.previous_attribute = najaeda.__dict__.pop("schematic", None)

    def tearDown(self):
        sys.modules.pop("najaeda.schematic", None)
        najaeda.__dict__.pop("schematic", None)
        if self.previous_module is not None:
            sys.modules["najaeda.schematic"] = self.previous_module
        if self.previous_attribute is not None:
            najaeda.schematic = self.previous_attribute

    def test_missing_viewer(self):
        with patch.dict(sys.modules, {"naja_schematic": None}):
            with self.assertRaisesRegex(ImportError, r"najaeda\[schematic\]") as error:
                importlib.import_module("najaeda.schematic")
        self.assertIsInstance(error.exception.__cause__, ImportError)

    def test_public_api(self):
        try:
            import naja_schematic
        except ImportError:
            self.skipTest("naja-schematic is not installed")
        schematic = importlib.import_module("najaeda.schematic")
        for name in ("show", "diagnosis_response", "handle_request", "__version__"):
            with self.subTest(name=name):
                self.assertIs(getattr(schematic, name), getattr(naja_schematic, name))


if __name__ == "__main__":
    unittest.main()
