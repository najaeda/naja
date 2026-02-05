# SPDX-FileCopyrightText: 2026 The Naja authors
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import faulthandler

from najaeda import netlist

verilog_benchmarks = os.environ.get('VERILOG_BENCHMARKS_PATH')

class NajaEDAPreprocessEnabledTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def testPreprocessEnabled(self):
        design_files = [os.path.join(verilog_benchmarks, "preprocess_top.v")]
        self.assertRaises(Exception, netlist.load_verilog, design_files)
        netlist.reset()
        top = netlist.load_verilog(
            design_files,
            config=netlist.VerilogConfig(preprocess_enabled=True),
        )
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())
        self.assertEqual(1, top.count_child_instances())
        inst = top.get_child_instance("u0")
        self.assertIsNotNone(inst)
        self.assertEqual("child", inst.get_model_name())
        term = top.get_term("a")
        self.assertIsNotNone(term)
        self.assertEqual(2, term.get_width())
        self.assertEqual(1, term.get_msb())
        self.assertEqual(0, term.get_lsb())

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
