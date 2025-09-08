# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import faulthandler
from najaeda import netlist
from najaeda import naja

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

class NajaEDANetlistTestAttributes(unittest.TestCase):
    def setUp(self):
        design_files = [os.path.join(verilog_benchmarks, "test_attributes.v")]
        #primitives = [os.path.join(liberty_benchmarks, "asap7_excerpt" , "test0.lib")]
        #netlist.load_liberty(primitives)
        netlist.load_verilog(design_files)

    def tearDown(self):
        netlist.reset()
    
    def test(self):
        top = netlist.get_top()
        self.assertIsNotNone(top)
        top_attributes = list(top.get_attributes())
        self.assertEqual(3, len(top_attributes))
        # (* MODULE_ATTRIBUTE = "Top level simple_netlist module", MODULE_VERSION = "1.0" *)
        # (* VERSION = 3 *)
        self.assertEqual('MODULE_ATTRIBUTE', top_attributes[0].get_name())
        self.assertEqual('Top level simple_netlist module', top_attributes[0].get_value())
        self.assertEqual('MODULE_VERSION', top_attributes[1].get_name())
        self.assertEqual('1.0', top_attributes[1].get_value())
        self.assertEqual('VERSION', top_attributes[2].get_name())
        self.assertEqual('3', top_attributes[2].get_value())

        and2_inst = top.get_child_instance('and2_inst')
        self.assertIsNotNone(and2_inst)
        self.assertEqual(3, and2_inst.count_attributes())
        and2_attributes = list(and2_inst.get_attributes())
        self.assertEqual(3, len(and2_attributes))
        self.assertEqual('INSTANCE_ATTRIBUTE_AND', and2_attributes[0].get_name())
        self.assertTrue(and2_attributes[0].has_value())
        self.assertEqual('and2_inst', and2_attributes[0].get_value())
        self.assertEqual('description', and2_attributes[1].get_name())
        self.assertTrue(and2_attributes[1].has_value())
        self.assertEqual('2-input AND gate instance', and2_attributes[1].get_value())
        self.assertEqual('VERSION', and2_attributes[2].get_name())
        self.assertTrue(and2_attributes[2].has_value())
        self.assertEqual('3', and2_attributes[2].get_value())

        #   get attributes on term
        # (* INPUT_ATTRIBUTE_A = "Input signal A" *)
        top_a = top.get_term('a')
        self.assertIsNotNone(top_a)
        top_a_attributes = list(top_a.get_attributes())
        self.assertEqual(1, len(top_a_attributes))
        self.assertEqual("INPUT_ATTRIBUTE_A", top_a_attributes[0].get_name())
        self.assertEqual("Input signal A", top_a_attributes[0].get_value())

        # (* WIRE_ATTRIBUTE = "Wire connecting AND gate output to top output" *)
        top_and_wire = top.get_net('and_wire')
        self.assertIsNotNone(top_and_wire)
        top_and_wire_attributes = list(top_and_wire.get_attributes())
        self.assertEqual(1, len(top_and_wire_attributes))
        self.assertEqual("WIRE_ATTRIBUTE", top_and_wire_attributes[0].get_name())
        self.assertEqual("Wire connecting AND gate output to top output", top_and_wire_attributes[0].get_value())

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
