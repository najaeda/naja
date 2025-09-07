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
        design_files = [os.path.join(verilog_benchmarks, "fulladder.v")]
        netlist.load_verilog(design_files)
        self.top = netlist.get_top()

    def tearDown(self):
        netlist.reset()
    
    def test(self):
        self.assertIsNotNone(self.top)
        self.assertEqual(self.top.get_name(), 'fulladder')
        self.assertEqual(5, self.top.count_terms())
        self.assertEqual(3, self.top.count_input_terms())
        self.assertEqual(2, self.top.count_output_terms())
        self.assertEqual(3, self.top.count_child_instances())
        ha1 = self.top.get_child_instance('ha1')
        self.assertIsNotNone(ha1)
        ha2 = self.top.get_child_instance('ha2')
        self.assertIsNotNone(ha2)
        self.assertEqual(ha1.get_model_name(), 'halfadder')
        self.assertEqual(ha2.get_model_name(), 'halfadder')

    def test_set_name(self):
        #set_name on instance under top
        ha1 = self.top.get_child_instance('ha1')
        ha1.set_name('ha1_renamed')
        ha1_renamed = self.top.get_child_instance('ha1_renamed')
        self.assertIsNotNone(ha1_renamed)
        self.assertEqual(ha1_renamed.get_model_name(), 'halfadder')

        #set_name on net under top
        sum1 = self.top.get_net('sum1')
        sum1.set_name('sum1_renamed')
        sum1_renamed = self.top.get_net('sum1_renamed')
        self.assertIsNotNone(sum1_renamed)
        self.assertEqual(sum1_renamed.get_name(), 'sum1_renamed')

        #set_name on instance under ha1
        ha1_carry_and = ha1.get_child_instance('carry_and')
        ha1_carry_and.set_name('carry_and_renamed')
        ha1_carry_and_renamed = ha1.get_child_instance('carry_and_renamed')
        #ha1 should have been uniquified
        self.assertNotEqual(ha1_carry_and.get_name(), 'carry_and')

        #set_name of net on instance under ha2
        ha2 = self.top.get_child_instance('ha2')
        self.assertIsNotNone(ha2)
        ha2_sum = ha2.get_net('sum')
        ha2_sum.set_name('sum_renamed')
        ha2_sum_renamed = ha2.get_net('sum_renamed')
        self.assertIsNotNone(ha2_sum_renamed)
        self.assertEqual(ha2_sum_renamed.get_name(), 'sum_renamed')

    def test_disconnect(self):
        #disconnect top term
        cin = self.top.get_term('cin')
        self.assertIsNotNone(cin)
        self.assertIsNone(cin.get_upper_net())
        self.assertIsNotNone(cin.get_lower_net())
        self.assertRaises(ValueError, cin.disconnect_upper_net)
        cin_lower_net = cin.get_lower_net()
        self.assertIsNotNone(cin_lower_net)
        cin.disconnect_lower_net()
        self.assertIsNone(cin.get_lower_net())
        cin.connect_lower_net(cin_lower_net)
        self.assertIsNotNone(cin.get_lower_net())
        self.assertEqual(cin.get_lower_net(), cin_lower_net)

        ha2 = self.top.get_child_instance('ha2')
        ha2_b = ha2.get_term('b')
        self.assertIsNotNone(ha2_b)
        self.assertIsNotNone(ha2_b.get_upper_net())
        self.assertEqual(ha2_b.get_upper_net(), cin_lower_net)
        self.assertIsNotNone(ha2_b.get_lower_net())
        ha2_b_lower_net = ha2_b.get_lower_net()
        ha2_b.disconnect_lower_net()
        self.assertIsNone(ha2_b.get_lower_net())
        ha2_b.connect_lower_net(ha2_b_lower_net)
        self.assertIsNotNone(ha2_b.get_lower_net())
        self.assertEqual(ha2_b.get_lower_net(), ha2_b_lower_net)
        ha2_b_lower_net.delete()
        print(ha2_b_lower_net)
        self.assertIsNone(ha2_b.get_lower_net())
        print(ha2)
        self.assertRaises(ValueError, self.top.delete)


    # def test_get_max_logic_level(self):
    #     # Test the maximum logic level of the design
    #     max_logic_level = netlist.get_max_logic_level()
    #     self.assertEqual(max_logic_level, 2)
    
    def test_get_fanout(self):
        # Test the maximum fanout of the design
        max_fanout = netlist.get_max_fanout()
        self.assertEqual(max_fanout[0], 2)

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
