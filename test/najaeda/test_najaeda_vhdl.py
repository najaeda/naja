# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
import tempfile
import unittest

from najaeda import netlist


class NajaEDAVHDLTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_load_vhdl(self):
        source = """\
entity inverter is port(a : in bit; y : out bit); end;
architecture rtl of inverter is begin y <= not a; end;
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "inverter.vhd"
            path.write_text(source, encoding="utf-8")
            top = netlist.load_vhdl(path)

        self.assertEqual("inverter", top.get_model_name())
        self.assertEqual(2, top.count_terms())
        self.assertEqual(1, top.count_input_terms())
        self.assertEqual(1, top.count_output_terms())

    def test_load_vhdl_hierarchy_with_explicit_top(self):
        source = """\
entity leaf is port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit; y : out bit); end;
architecture structural of top is begin
  u0: entity work.leaf port map(a, y);
end;
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "hierarchy.vhdl"
            path.write_text(source, encoding="utf-8")
            top = netlist.load_vhdl(path, top="TOP")

        self.assertEqual("top", top.get_model_name())
        self.assertEqual(1, top.count_child_instances())
        self.assertEqual("leaf", next(top.get_child_instances()).get_model_name())

    def test_load_vhdl_validates_arguments_before_native_loading(self):
        with self.assertRaisesRegex(TypeError, "VHDL file must be a path string"):
            netlist.load_vhdl(1)
        with self.assertRaisesRegex(ValueError, "VHDL file must not be empty"):
            netlist.load_vhdl(" ")
        with self.assertRaisesRegex(TypeError, "top must be a str or None"):
            netlist.load_vhdl("missing.vhd", top=1)
        with self.assertRaisesRegex(ValueError, "top must not be empty"):
            netlist.load_vhdl("missing.vhd", top=" ")

        missing = Path(tempfile.gettempdir()) / "najaeda-missing-vhdl-file.vhd"
        with self.assertRaises(FileNotFoundError) as context:
            netlist.load_vhdl(missing)
        self.assertIn(repr(str(missing)), str(context.exception))
        self.assertIn("resolved to", str(context.exception))

        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "VHDL input path is not a file"):
                netlist.load_vhdl(directory)


if __name__ == "__main__":
    unittest.main()
