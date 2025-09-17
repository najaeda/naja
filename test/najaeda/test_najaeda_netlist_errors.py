
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler

from najaeda import netlist
from najaeda import naja

class NajaNetlistTestErrors(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_missing_model(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        self.assertRaises(Exception, top.create_child_instance, 'Module0', 'mod')

    def test_missing_verilog(self):
        self.assertRaises(Exception, netlist.load_verilog)
        self.assertRaises(Exception, netlist.load_verilog, [])

    def test_width_mismatch(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        topTerm = top.create_input_term('Top')
        topNet = top.create_bus_net('net', 1, 0)
        self.assertRaises(Exception, topTerm.connect_upper_net, topNet)

    def test_empty_liberty(self):
        self.assertRaises(Exception, netlist.load_liberty, [])
            
if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
