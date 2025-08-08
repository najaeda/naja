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

        #rename nets
        net1 = top.get_net('net1')
        self.assertIsNotNone(net1)
        self.assertEqual('net1', net1.get_name())
        net1.set_name('mynet1')
        self.assertEqual('mynet1', net1.get_name())
        self.assertIsNone(top.get_net('net1'))
        self.assertIsNotNone(top.get_net('mynet1'))
        self.assertEqual(net1, top.get_net('mynet1'))

        #error on renaming a net concatenation
        net4 = top.get_net('net4')
        self.assertIsNotNone(net4)
        self.assertEqual('net4', net4.get_name())
        net4_1 = net4.get_bit(1)
        self.assertIsNotNone(net4_1)
        with self.assertRaises(ValueError):
            net4_1.set_name('my_net4')

        net4_0 = net4.get_bit(0)
        self.assertIsNotNone(net4_0)

        net_concat = netlist.Net('net_concat', net_concat=[net4_0, net4_1])
        self.assertIsNotNone(net_concat)
        self.assertTrue(net_concat.is_concat())
        with self.assertRaises(ValueError):
            net_concat.set_name('my_net_concat')


if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
