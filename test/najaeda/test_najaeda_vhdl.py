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
            with self.assertWarnsRegex(RuntimeWarning, "VHDL parser is in Beta mode"):
                top = netlist.load_vhdl(path)

        self.assertEqual("inverter", top.get_model_name())
        self.assertEqual(2, top.count_terms())
        self.assertEqual(1, top.count_input_terms())
        self.assertEqual(1, top.count_output_terms())

    def test_hdl_destination_library(self):
        with tempfile.TemporaryDirectory() as directory:
            vhdl = Path(directory) / "leaf.vhd"
            sv = Path(directory) / "leaf.sv"
            vhdl.write_text("entity leaf is port(a : in bit; y : out bit); end; "
                            "architecture rtl of leaf is begin y <= a; end;")
            sv.write_text("module leaf(input a, output y); assign y = a; endmodule")
            for loader, path in ((netlist.load_vhdl, vhdl),
                                 (netlist.load_system_verilog, str(sv))):
                with self.subTest(loader=loader.__name__):
                    for invalid in (None, 1):
                        with self.assertRaises(TypeError):
                            loader(path, library=invalid)
                    with self.assertRaises(ValueError):
                        loader(path, library="")
                    top = loader(path, library="Cells")
                    self.assertEqual(top.get_model_name(), "leaf")
                    self.assertIsNotNone(netlist.naja.NLUniverse.get().getTopDB().getLibrary("Cells"))
                    netlist.reset()

    def test_load_boolean_generic_default(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "boolean.vhd"
            path.write_text("entity top is generic(enabled : boolean := true); "
                            "port(a : in bit; y : out bit); end; "
                            "architecture rtl of top is begin y <= a when enabled else not a; end;")
            top = netlist.load_vhdl(path, library="BooleanCells", diagnostics_report_path=None)
            self.assertEqual(top.get_model_name(), "top")
            self.assertEqual(top.count_terms(), 2)

    def test_load_vhdl_warning_report(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "diagnostic.vhd"
            report = Path(directory) / "reports" / "warnings.log"
            path.write_text("entity e is port(y : out bit); end; "
                            "architecture rtl of e is begin assert false; "
                            "assert false; y <= '1'; end;", encoding="utf-8")
            top = netlist.load_vhdl(path, diagnostics_report_path=report)
            self.assertEqual(top.get_model_name(), "e")
            self.assertEqual(report.read_text().count("[ignored-assertion]"), 2)
            netlist.reset()
            netlist.load_vhdl(path, diagnostics_report_path=None)
            with self.assertRaises(TypeError):
                netlist.load_vhdl(path, diagnostics_report_path=1)
            with self.assertRaises(ValueError):
                netlist.load_vhdl(path, diagnostics_report_path="")

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

    def test_load_vhdl_rejects_bytes_report_pathlike(self):
        class BytesPath:
            def __fspath__(self):
                return b"warnings.log"

        with self.assertRaisesRegex(TypeError, "diagnostics_report_path must be a path string"):
            netlist.load_vhdl("missing.vhd", diagnostics_report_path=BytesPath())


if __name__ == "__main__":
    unittest.main()
