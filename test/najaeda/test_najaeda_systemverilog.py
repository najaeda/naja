# SPDX-FileCopyrightText: 2026 The Naja authors
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest
import faulthandler

from najaeda import netlist

systemverilog_benchmarks = os.environ.get("SYSTEMVERILOG_BENCHMARKS_PATH")
if not systemverilog_benchmarks:
    verilog_benchmarks = os.environ.get("VERILOG_BENCHMARKS_PATH")
    if verilog_benchmarks:
        systemverilog_benchmarks = verilog_benchmarks.replace(
            "/verilog/benchmarks", "/systemverilog/benchmarks")
najaeda_test_path = os.environ.get("NAJAEDA_TEST_PATH")
if not najaeda_test_path:
    najaeda_test_path = os.getcwd()


class NajaEDASystemVerilogTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_load_systemverilog(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_systemverilog(design_files)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())
        self.assertEqual(3, top.count_terms())
        self.assertEqual(2, top.count_input_terms())
        self.assertEqual(1, top.count_output_terms())

    def test_load_systemverilog_with_ast_json_dump(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        json_path = os.path.join(najaeda_test_path, "simple_elaborated_ast_najaeda.json")
        diagnostics_path = os.path.join(najaeda_test_path, "simple_diagnostics_najaeda.txt")
        top = netlist.load_systemverilog(
            design_files,
            config=netlist.SystemVerilogConfig(
                elaborated_ast_json_path=json_path,
                diagnostics_report_path=diagnostics_path),
        )
        self.assertIsNotNone(top)
        self.assertTrue(os.path.exists(json_path))
        self.assertTrue(os.path.exists(diagnostics_path))

    def test_load_systemverilog_with_flist(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        flist_path = os.path.join(najaeda_test_path, "simple_najaeda.f")
        with open(flist_path, "w", encoding="utf-8") as flist:
            flist.write(f"{design_file}\n")
        top = netlist.load_systemverilog(
            [],
            config=netlist.SystemVerilogConfig(flist=flist_path),
        )
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_load_system_verilog_alias(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_load_systemverilog_with_top(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            generic_sv = os.path.join(temp_dir, "generic.sv")
            with open(generic_sv, "w", encoding="utf-8") as generic_file:
                generic_file.write(
                    "module generic #(parameter type T = logic) "
                    "(input T i, output logic y);\n"
                    "  assign y = i.foo;\n"
                    "endmodule\n")

            top_sv = os.path.join(temp_dir, "top2.sv")
            with open(top_sv, "w", encoding="utf-8") as top_file:
                top_file.write(
                    "module top2(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n")

            top = netlist.load_systemverilog(
                [generic_sv, top_sv],
                config=netlist.SystemVerilogConfig(top="top2"),
            )
            self.assertIsNotNone(top)
            self.assertEqual("top2", top.get_model_name())

    def test_load_systemverilog_with_non_string_top_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                ValueError,
                r"SystemVerilogConfig\.top must be a str \(got int\)"):
            netlist.load_systemverilog(
                design_files,
                config=netlist.SystemVerilogConfig(top=123),
            )

    def test_load_systemverilog_with_flist_and_top(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            generic_sv = os.path.join(temp_dir, "generic.sv")
            with open(generic_sv, "w", encoding="utf-8") as generic_file:
                generic_file.write(
                    "module generic #(parameter type T = logic) "
                    "(input T i, output logic y);\n"
                    "  assign y = i.foo;\n"
                    "endmodule\n")

            top_sv = os.path.join(temp_dir, "top2.sv")
            with open(top_sv, "w", encoding="utf-8") as top_file:
                top_file.write(
                    "module top2(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n")

            source_flist = os.path.join(temp_dir, "sources.f")
            with open(source_flist, "w", encoding="utf-8") as flist:
                flist.write(f"{generic_sv}\n")
                flist.write(f"{top_sv}\n")

            top = netlist.load_systemverilog(
                [],
                config=netlist.SystemVerilogConfig(
                    flist=source_flist,
                    top="top2"),
            )
            self.assertIsNotNone(top)
            self.assertEqual("top2", top.get_model_name())


if __name__ == "__main__":
    faulthandler.enable()
    unittest.main()
