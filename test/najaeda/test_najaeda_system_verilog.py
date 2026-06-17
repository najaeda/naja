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

    def test_load_system_verilog(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())
        self.assertEqual(3, top.count_terms())
        self.assertEqual(2, top.count_input_terms())
        self.assertEqual(1, top.count_output_terms())
        source_range = top.get_source_range()
        self.assertIsInstance(source_range, netlist.SourceRange)
        self.assertTrue(source_range.file.endswith(
            "systemverilog/benchmarks/simple/simple.sv"))
        self.assertGreaterEqual(source_range.line, 1)
        self.assertGreaterEqual(source_range.column, 1)
        self.assertGreaterEqual(source_range.end_line, source_range.line)
        self.assertGreaterEqual(source_range.end_column, 1)

        term_range = top.get_term("a").get_source_range()
        self.assertIsInstance(term_range, netlist.SourceRange)
        self.assertTrue(term_range.file.endswith(
            "systemverilog/benchmarks/simple/simple.sv"))

        net_range = top.get_net("y").get_source_range()
        self.assertIsInstance(net_range, netlist.SourceRange)
        self.assertTrue(net_range.file.endswith(
            "systemverilog/benchmarks/simple/simple.sv"))

    def test_load_system_verilog_single_arg(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        top = netlist.load_system_verilog(design_file)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_source_range_absent(self):
        top = netlist.create_top("top")
        self.assertIsNone(top.get_source_range())

        concat_net = netlist.Net([], net_concat=[])
        self.assertIsNone(concat_net.get_source_range())

    def test_load_system_verilog_with_ast_json_dump(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        json_path = os.path.join(najaeda_test_path, "simple_elaborated_ast_najaeda.json")
        diagnostics_path = os.path.join(najaeda_test_path, "simple_diagnostics_najaeda.txt")
        top = netlist.load_system_verilog(
            design_files,
            config=netlist.SystemVerilogConfig(
                elaborated_ast_json_path=json_path,
                diagnostics_report_path=diagnostics_path),
        )
        self.assertIsNotNone(top)
        self.assertTrue(os.path.exists(json_path))
        self.assertTrue(os.path.exists(diagnostics_path))

    def test_dump_verilog_with_config(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)

        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as dump_dir:
            without_rtl_infos = os.path.join(dump_dir, "simple_default_no_rtl_infos.v")
            top.dump_verilog(without_rtl_infos)
            with open(without_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertNotIn("sv_src_file", dumped_text)

            with_rtl_infos = os.path.join(dump_dir, "simple_with_rtl_infos.v")
            top.dump_verilog(
                with_rtl_infos,
                config=netlist.VerilogDumpConfig(dumpRTLInfosAsAttributes=True),
            )
            with open(with_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertNotIn("sv_src_file", dumped_text)
            self.assertIn('naja_sv_src="', dumped_text)

            with_verbose_rtl_infos = os.path.join(
                dump_dir, "simple_with_verbose_rtl_infos.v"
            )
            top.dump_verilog(
                with_verbose_rtl_infos,
                config=netlist.VerilogDumpConfig(
                    dumpRTLInfosAsAttributes=True,
                    rtlInfoDumpMode="VerboseAttributes",
                ),
            )
            with open(with_verbose_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertIn("sv_src_file", dumped_text)
            self.assertNotIn('naja_sv_src="', dumped_text)

            with_assign_instances = os.path.join(dump_dir, "simple_with_assign_instances.v")
            top.dump_verilog(
                with_assign_instances,
                config=netlist.VerilogDumpConfig(dumpAssignsAsInstances=True),
            )
            with open(with_assign_instances, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertIn("assign_module", dumped_text)

    def test_dump_verilog_config_rejects_invalid_rtl_info_mode(self):
        with self.assertRaises(ValueError) as context:
            netlist.VerilogDumpConfig(rtlInfoDumpMode="Invalid")
        self.assertIn("Invalid rtlInfoDumpMode", str(context.exception))

    def test_load_system_verilog_with_flist(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        flist_path = os.path.join(najaeda_test_path, "simple_najaeda.f")
        with open(flist_path, "w", encoding="utf-8") as flist:
            flist.write(f"{design_file}\n")
        top = netlist.load_system_verilog(
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

    def test_load_system_verilog_with_top(self):
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

            top = netlist.load_system_verilog(
                [generic_sv, top_sv],
                config=netlist.SystemVerilogConfig(top="top2"),
            )
            self.assertIsNotNone(top)
            self.assertEqual("top2", top.get_model_name())

    def test_load_system_verilog_with_non_string_top_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                ValueError,
                r"SystemVerilogConfig\.top must be a str \(got int\)"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(top=123),
            )

    def test_load_system_verilog_with_empty_top_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                ValueError,
                r"SystemVerilogConfig\.top must not be empty"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(top=""),
            )

    def test_load_system_verilog_with_invalid_defines_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        cases = [
            (
                "SYNTHESIS",
                r"SystemVerilogConfig\.defines must be a list \(got str\)",
            ),
            (
                [1],
                r"SystemVerilogConfig\.defines items must be strings",
            ),
            (
                [""],
                r"SystemVerilogConfig\.defines items must not be empty",
            ),
            (
                ["HAS SPACE"],
                r"SystemVerilogConfig\.defines items must not contain whitespace",
            ),
        ]
        for defines, message in cases:
            with self.subTest(defines=defines):
                with self.assertRaisesRegex(ValueError, message):
                    netlist.load_system_verilog(
                        design_files,
                        config=netlist.SystemVerilogConfig(defines=defines),
                    )

    def test_load_system_verilog_with_flist_top_and_define(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            design_sv = os.path.join(temp_dir, "define_selects_top.sv")
            with open(design_sv, "w", encoding="utf-8") as design_file:
                design_file.write(
                    "`ifdef SYNTHESIS\n"
                    "module synth_top(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n"
                    "`else\n"
                    "module sim_top(input logic a, output logic y);\n"
                    "  assign y = ~a;\n"
                    "endmodule\n"
                    "`endif\n")

            source_flist = os.path.join(temp_dir, "sources.f")
            with open(source_flist, "w", encoding="utf-8") as flist:
                flist.write(f"{design_sv}\n")

            top = netlist.load_system_verilog(
                [],
                config=netlist.SystemVerilogConfig(
                    flist=source_flist,
                    top="synth_top",
                    defines=["SYNTHESIS"]),
            )
            self.assertIsNotNone(top)
            self.assertEqual("synth_top", top.get_model_name())

    def test_load_system_verilog_with_flist_and_top(self):
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

            top = netlist.load_system_verilog(
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
