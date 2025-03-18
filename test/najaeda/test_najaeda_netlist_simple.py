
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler

from najaeda import netlist
from najaeda import naja

class NajaNetlistTestErrors(unittest.TestCase):
    def tearDown(self):
        if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()

    def test_terms(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        top.create_input_term('in_term')
        top.create_output_term('out_term')
        top.create_inout_term('inout_term')
        top.create_input_bus_term('in_bus', 1, 0)
        top.create_output_bus_term('out_bus', 0, 1)
        top.create_inout_bus_term('inout_bus', 1, 1)

        self.assertIsNotNone(top.get_term('in_term'))
        self.assertEqual(top.get_term('in_term').get_name(), 'in_term')
        self.assertEqual(top.get_term('in_term').get_direction(), netlist.Term.INPUT)
        self.assertEqual(top.get_term('out_term').get_direction(), netlist.Term.OUTPUT)
        self.assertEqual(top.get_term('inout_term').get_direction(), netlist.Term.INOUT)
        self.assertTrue(top.get_term('in_term').is_scalar())
        self.assertTrue(top.get_term('out_term').is_scalar())
        self.assertTrue(top.get_term('inout_term').is_scalar())
        self.assertEqual(top.get_term('in_bus').get_direction(), netlist.Term.INPUT)
        self.assertEqual(top.get_term('out_bus').get_direction(), netlist.Term.OUTPUT)
        self.assertEqual(top.get_term('inout_bus').get_direction(), netlist.Term.INOUT)
        self.assertTrue(top.get_term('in_bus').is_bus())
        self.assertTrue(top.get_term('out_bus').is_bus())
        self.assertTrue(top.get_term('inout_bus').is_bus())
        self.assertEqual(top.get_term('in_bus').get_msb(), 1)
        self.assertEqual(top.get_term('in_bus').get_lsb(), 0)
        self.assertEqual(top.get_term('out_bus').get_msb(), 0)
        self.assertEqual(top.get_term('out_bus').get_lsb(), 1)
        self.assertEqual(top.get_term('inout_bus').get_msb(), 1)
        self.assertEqual(top.get_term('inout_bus').get_lsb(), 1)

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
