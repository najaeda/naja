# SPDX-FileCopyrightText: 2026 The Naja authors
#
# SPDX-License-Identifier: Apache-2.0

import os
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
        top = netlist.load_systemverilog(
            design_files,
            config=netlist.SystemVerilogConfig(elaborated_ast_json_path=json_path),
        )
        self.assertIsNotNone(top)
        self.assertTrue(os.path.exists(json_path))

    def test_load_system_verilog_alias(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())


if __name__ == "__main__":
    faulthandler.enable()
    unittest.main()
