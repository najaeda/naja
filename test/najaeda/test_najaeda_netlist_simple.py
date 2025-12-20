
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler

from najaeda import netlist
from najaeda import naja

class NajaNetlistTestSimple(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_versioning(self):
        import najaeda
        version = najaeda.version()
        self.assertIsNotNone(version)
        hash = najaeda.git_hash()
        self.assertIsNotNone(hash)

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
        self.assertEqual(top.get_term('in_term').get_direction(), netlist.Term.Direction.INPUT)
        self.assertEqual(top.get_term('out_term').get_direction(), netlist.Term.Direction.OUTPUT)
        self.assertEqual(top.get_term('inout_term').get_direction(), netlist.Term.Direction.INOUT)
        self.assertTrue(top.get_term('in_term').is_scalar())
        self.assertTrue(top.get_term('out_term').is_scalar())
        self.assertTrue(top.get_term('inout_term').is_scalar())
        self.assertEqual(top.get_term('in_bus').get_direction(), netlist.Term.Direction.INPUT)
        self.assertEqual(top.get_term('out_bus').get_direction(), netlist.Term.Direction.OUTPUT)
        self.assertEqual(top.get_term('inout_bus').get_direction(), netlist.Term.Direction.INOUT)
        self.assertTrue(top.get_term('in_bus').is_bus())
        self.assertTrue(top.get_term('out_bus').is_bus())
        self.assertTrue(top.get_term('inout_bus').is_bus())
        self.assertEqual(top.get_term('in_bus').get_msb(), 1)
        self.assertEqual(top.get_term('in_bus').get_lsb(), 0)
        self.assertEqual(top.get_term('out_bus').get_msb(), 0)
        self.assertEqual(top.get_term('out_bus').get_lsb(), 1)
        self.assertEqual(top.get_term('inout_bus').get_msb(), 1)
        self.assertEqual(top.get_term('inout_bus').get_lsb(), 1)

    def test_equipotential_on_bus_term_error(self):
        top = netlist.create_top('Top')
        bus_term = top.create_input_bus_term('in_bus', 3, 0)
        with self.assertRaises(ValueError):
            bus_term.get_equipotential()

    def test_comparison(self):
        top = netlist.create_top('Top')
        term1 = top.create_input_term('in_term1')
        term2 = top.create_input_bus_term('in_term2', 31, 0)

        net1 = top.create_net('net1')
        net2 = top.create_bus_net('net2', 31, 0)

        term1.connect_lower_net(net1)
        term2.connect_lower_net(net2)

        self.assertEqual(net1, net1)
        self.assertNotEqual(net1, net2)
        self.assertEqual(term1, term1)
        self.assertNotEqual(term1, term2)
        self.assertNotEqual(term1, net1)
        self.assertNotEqual(net1, term1)
        self.assertTrue(net1 != term1)

        self.assertLess(term1, term2)
        self.assertGreater(term2, term1)

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
