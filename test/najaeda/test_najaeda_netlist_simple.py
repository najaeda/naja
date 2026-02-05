
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
        self.assertNotEqual(net1, term1)

        with self.assertRaises(TypeError):
            term1 < net1
        with self.assertRaises(TypeError):
            net1 < term1
        with self.assertRaises(TypeError):
            term1 > net1
        with self.assertRaises(TypeError):
            net1 > term1
        with self.assertRaises(TypeError):
            term1 <= net1
        with self.assertRaises(TypeError):
            net1 <= term1
        with self.assertRaises(TypeError):
            term1 >= net1
        with self.assertRaises(TypeError):
            net1 >= term1

        self.assertLess(term1, term2)
        self.assertGreater(term2, term1)
        self.assertNotEqual(term1, net1)

        term1_bit2 = term2.get_bit(2)
        term1_bit10 = term2.get_bit(10)
        self.assertNotEqual(term1_bit2, term1_bit10)
        self.assertLess(term1_bit2, term1_bit10)
        self.assertGreater(term1_bit10, term1_bit2)
        self.assertLess(term1, term1_bit2)
        self.assertGreater(term1_bit2, term1)
        self.assertLess(term1, term1_bit10)
        self.assertGreater(term1_bit10, term1)

    def test_term_delete(self):
        top = netlist.create_top('Top')
        scalar = top.create_input_term('in_term')
        bus = top.create_input_bus_term('bus', 3, 0)

        self.assertEqual(2, top.count_terms())
        self.assertEqual(5, top.count_bit_terms())

        scalar.delete()
        self.assertIsNone(top.get_term('in_term'))
        self.assertEqual(1, top.count_terms())
        self.assertEqual(4, top.count_bit_terms())

        bit2 = bus.get_bit(2)
        self.assertIsNotNone(bit2)
        bit2.delete()
        self.assertEqual(3, bus.get_msb())
        self.assertEqual(0, bus.get_lsb())
        self.assertEqual(4, bus.get_width())
        bit_numbers = [b.get_bit_number() for b in bus.get_bits()]
        self.assertNotIn(2, bit_numbers)
        self.assertEqual(3, top.count_bit_terms())

        bus.delete()
        self.assertIsNone(top.get_term('bus'))
        self.assertEqual(0, top.count_terms())
        self.assertEqual(0, top.count_bit_terms())

    def test_term_delete_intermediate_hierarchy(self):
        top = netlist.create_top('Top')
        top_design = naja.NLUniverse.get().getTopDesign()
        lib = top_design.getLibrary()
        model1 = naja.SNLDesign.create(lib, "M1")
        model2 = naja.SNLDesign.create(lib, "M2")

        scalar = naja.SNLScalarTerm.create(model2, naja.SNLTerm.Direction.Input, "in_term")
        bus = naja.SNLBusTerm.create(model2, naja.SNLTerm.Direction.Input, 3, 0, "bus")

        inst2 = naja.SNLInstance.create(model1, model2, "u2")
        inst1 = naja.SNLInstance.create(top_design, model1, "u1")
        path1 = naja.SNLPath(inst1)
        path2 = naja.SNLPath(path1, inst2)
        inst2_wrapped = netlist.Instance(path2)

        self.assertEqual(2, inst2_wrapped.count_terms())
        self.assertEqual(5, inst2_wrapped.count_bit_terms())

        inst2_wrapped.get_term("in_term").delete()
        self.assertIsNone(inst2_wrapped.get_term("in_term"))
        self.assertEqual(1, inst2_wrapped.count_terms())
        self.assertEqual(4, inst2_wrapped.count_bit_terms())

        bit2 = inst2_wrapped.get_term("bus").get_bit(2)
        self.assertIsNotNone(bit2)
        bit2.delete()
        self.assertEqual(3, inst2_wrapped.get_term("bus").get_msb())
        self.assertEqual(0, inst2_wrapped.get_term("bus").get_lsb())
        bit_numbers = [b.get_bit_number() for b in inst2_wrapped.get_term("bus").get_bits()]
        self.assertNotIn(2, bit_numbers)
        self.assertEqual(3, inst2_wrapped.count_bit_terms())

        inst2_wrapped.get_term("bus").delete()
        self.assertIsNone(inst2_wrapped.get_term("bus"))
        self.assertEqual(0, inst2_wrapped.count_terms())
        self.assertEqual(0, inst2_wrapped.count_bit_terms())

    def test_term_set_msb_lsb(self):
        top = netlist.create_top('Top')
        scalar = top.create_input_term('in_term')
        bus = top.create_input_bus_term('bus', 3, 0)

        bus.set_msb(2)
        self.assertEqual(2, bus.get_msb())
        self.assertEqual(0, bus.get_lsb())
        bus.set_lsb(1)
        self.assertEqual(2, bus.get_msb())
        self.assertEqual(1, bus.get_lsb())

        with self.assertRaises(ValueError):
            scalar.set_msb(0)
        with self.assertRaises(ValueError):
            scalar.set_lsb(0)
        with self.assertRaises(ValueError):
            bus.get_bit(1).set_msb(1)
        with self.assertRaises(ValueError):
            bus.get_bit(1).set_lsb(1)

    def test_net_set_msb_lsb(self):
        top = netlist.create_top('Top')
        scalar_net = top.create_net('n0')
        bus_net = top.create_bus_net('n1', 3, 0)

        bus_net.set_msb(2)
        self.assertEqual(2, bus_net.get_msb())
        self.assertEqual(0, bus_net.get_lsb())
        bus_net.set_lsb(1)
        self.assertEqual(2, bus_net.get_msb())
        self.assertEqual(1, bus_net.get_lsb())

        with self.assertRaises(ValueError):
            scalar_net.set_msb(0)
        with self.assertRaises(ValueError):
            scalar_net.set_lsb(0)
        with self.assertRaises(ValueError):
            bus_net.get_bit(1).set_msb(1)
        with self.assertRaises(ValueError):
            bus_net.get_bit(1).set_lsb(1)
        net_a = top.create_net('n2')
        net_b = top.create_net('n3')
        concat = netlist.Net([], net_concat=[net_a.net, net_b.net])
        with self.assertRaises(ValueError):
            concat.set_msb(1)
        with self.assertRaises(ValueError):
            concat.set_lsb(0)

    def test_net_set_msb_lsb_intermediate(self):
        top = netlist.create_top('Top')
        top_design = naja.NLUniverse.get().getTopDesign()
        lib = top_design.getLibrary()
        model = naja.SNLDesign.create(lib, "M1")
        naja.SNLBusNet.create(model, 3, 0, "bus")
        inst = top.create_child_instance("M1", "u1")
        net = inst.get_net("bus")
        self.assertIsNotNone(net)
        net.set_msb(2)
        self.assertEqual(2, net.get_msb())
        net.set_lsb(1)
        self.assertEqual(1, net.get_lsb())

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
