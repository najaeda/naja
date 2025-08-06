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

liberty_benchmarks = os.environ.get('LIBERTY_BENCHMARKS_PATH')
verilog_benchmarks = os.environ.get('VERILOG_BENCHMARKS_PATH')

class NajaNetlistTestRename(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        netlist.reset()
    
    def test_rename(self):
        design_files = [os.path.join(verilog_benchmarks, "test0.v")]
        primitives = [os.path.join(liberty_benchmarks, "asap7_excerpt" , "test0.lib")]
        netlist.load_liberty(primitives)
        netlist.load_verilog(design_files)
        top = netlist.get_top()
        self.assertIsNotNone(top)
        inst0 = top.get_child_instance('inst0')
        self.assertIsNotNone(inst0)
        self.assertEqual('inst0', inst0.get_name())
        inst0.set_name('myinst0')
        self.assertEqual('myinst0', inst0.get_name())
        self.assertIsNone(top.get_child_instance('inst0'))
        self.assertIsNotNone(top.get_child_instance('myinst0'))
        self.assertEqual(inst0, top.get_child_instance('myinst0'))

        #rename top
        self.assertEqual('test', top.get_name())
        top.set_name('mytest')
        self.assertEqual('mytest', top.get_name())

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
