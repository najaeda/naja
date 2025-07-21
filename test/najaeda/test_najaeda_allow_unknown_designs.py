# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import faulthandler

from najaeda import netlist

# Get the PYTHONPATH environment variable
pythonpath = os.environ.get('PYTHONPATH')

if pythonpath:
    # os.pathsep is the separator used in PYTHONPATH (':' on Unix, ';' on Windows)
    paths = pythonpath.split(os.pathsep)
    print("PYTHONPATH contains the following directories:")
    for path in paths:
        print(path)
else:
    print("PYTHONPATH is not set.")

verilog_benchmarks = os.environ.get('VERILOG_BENCHMARKS_PATH')

class NajaEDAUnknownDesignsTest0(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        netlist.reset()
    
    def testWithOption(self):
        design_files = [os.path.join(verilog_benchmarks, "auto_blackbox_test0.v")]
        netlist.load_verilog(design_files, config=netlist.VerilogConfig(allow_unknown_designs=True))
        top = netlist.get_top()
        self.assertIsNotNone(top)
        #auto_blackbox0 ins0(
        #.A(1'b0),
        #.B(1'b1),
        #.C({1'b0, 1'b1, 1'b0}),
        #.OUT({net[0], net[1]}),
        #.COUT(net[2])
        #);
        self.assertEqual(1, top.count_child_instances())
        ins0 = top.get_child_instance('ins0')
        self.assertEqual('auto_blackbox0', ins0.get_model_name())
        self.assertIsNotNone(ins0)
        self.assertTrue(ins0.is_blackbox())
        self.assertEqual(5, ins0.count_terms())

        a = ins0.get_term('A')
        self.assertIsNotNone(a)
        self.assertTrue(a.is_scalar())
        self.assertEqual(a.get_direction(), netlist.Term.Direction.INPUT)

        b = ins0.get_term('B')
        self.assertIsNotNone(b)
        self.assertTrue(b.is_scalar())
        self.assertEqual(b.get_direction(), netlist.Term.Direction.INPUT)

    def testWithoutOption(self):
        design_files = [os.path.join(verilog_benchmarks, "auto_blackbox_test0.v")]
        self.assertRaises(Exception, netlist.load_verilog, design_files)

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
