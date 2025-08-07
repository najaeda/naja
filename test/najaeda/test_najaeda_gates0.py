# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
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

liberty_benchmarks = os.environ.get('LIBERTY_BENCHMARKS_PATH')
verilog_benchmarks = os.environ.get('VERILOG_BENCHMARKS_PATH')

class NajaNetlistTestGates0(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        netlist.reset()
    
    def test(self):
        design_files = [os.path.join(verilog_benchmarks, "test_gates0.v")]
        netlist.load_verilog(design_files)
        top = netlist.get_top()
        self.assertIsNotNone(top)
        and0 = top.get_child_instance('and0')
        self.assertIsNotNone(and0)
        self.assertEqual(and0.get_model_name(), 'and_2')
        self.assertEqual(2, and0.count_terms())
        self.assertEqual(1, and0.count_input_terms())
        self.assertEqual(1, and0.count_output_terms())
        for term in and0.get_terms():
            self.assertIsNotNone(term)
            self.assertTrue(term.is_unnamed())

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
