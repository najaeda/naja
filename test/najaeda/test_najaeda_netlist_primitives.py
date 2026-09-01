
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler

from najaeda import netlist
from najaeda import naja
from najaeda.primitives import utils


class NajaNetlistTestPrimitives(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_bit_terms_scalar_and_none(self):
        term = object()
        self.assertEqual([], utils._bit_terms(None))
        self.assertEqual([term], utils._bit_terms(term))

    def test_yosys_primitives(self):
        netlist.load_primitives('yosys')
        top = netlist.create_top('Top')

        and2_ins = top.create_child_instance('$_AND_', 'and2_ins')
        or2_ins = top.create_child_instance('$_OR_', 'or2_ins')
        self.assertIsNotNone(and2_ins)
        self.assertIsNotNone(or2_ins)

        library = naja.NLUniverse.get().getTopDB().getLibrary("yosys")
        dff = library.getSNLDesign("$_DFF_P_")
        self.assertTrue(dff.getScalarTerm("C").is_clock())
        self.assertTrue(dff.getScalarTerm("D").is_data_input())
        self.assertTrue(dff.getScalarTerm("Q").is_data_output())
        self.assertEqual(
            [dff.getScalarTerm("D")],
            list(dff.getClockRelatedInputs(dff.getScalarTerm("C"))),
        )

        async_reset = library.getSNLDesign("$_DFFE_PN0N_")
        self.assertTrue(async_reset.getScalarTerm("E").is_enable())
        self.assertTrue(async_reset.getScalarTerm("R").is_async_reset())
        self.assertEqual(
            naja.SNLActiveLevel.Low,
            async_reset.getScalarTerm("R").getResetActiveLevel(),
        )

        sync_set = library.getSNLDesign("$_SDFFCE_PP1P_")
        self.assertTrue(sync_set.getScalarTerm("E").is_enable())
        self.assertTrue(sync_set.getScalarTerm("R").is_sync_set())
        self.assertEqual(
            naja.SNLActiveLevel.High,
            sync_set.getScalarTerm("R").getResetActiveLevel(),
        )

    def test_gate_family_predicates(self):
        top = netlist.create_top('Top')
        primitives = naja.NLLibrary.createPrimitives(
            naja.NLUniverse.get().getTopDB(), "gate_primitives")

        def create_gate(name, mask):
            prim = naja.SNLDesign.createPrimitive(primitives, name)
            naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "A")
            naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "B")
            naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
            prim.setTruthTable(mask)
            return top.create_child_instance(name, f"{name.lower()}_ins")

        and2_ins = create_gate("AND2", 0x8)
        nand2_ins = create_gate("NAND2", 0x7)
        or2_ins = create_gate("OR2", 0xE)
        nor2_ins = create_gate("NOR2", 0x1)
        xor2_ins = create_gate("XOR2", 0x6)
        xnor2_ins = create_gate("XNOR2", 0x9)

        self.assertTrue(and2_ins.is_and())
        self.assertFalse(and2_ins.is_nand())
        self.assertTrue(nand2_ins.is_nand())
        self.assertTrue(or2_ins.is_or())
        self.assertTrue(nor2_ins.is_nor())
        self.assertTrue(xor2_ins.is_xor())
        self.assertTrue(xnor2_ins.is_xnor())

    def test_xilinx_primitives(self):
        top = netlist.create_top('Top')
        i = top.create_input_term("I")
        o = top.create_output_term("O")
        self.assertIsNotNone(top)
        netlist.load_primitives('xilinx')
        library = naja.NLUniverse.get().getTopDB().getLibrary("xilinx")

        fdce = library.getSNLDesign("FDCE")
        self.assertTrue(fdce.getScalarTerm("C").is_clock())
        self.assertTrue(fdce.getScalarTerm("D").is_data_input())
        self.assertTrue(fdce.getScalarTerm("Q").is_data_output())
        self.assertTrue(fdce.getScalarTerm("CE").is_enable())
        self.assertTrue(fdce.getScalarTerm("CLR").is_async_reset())
        self.assertEqual(
            naja.SNLActiveLevel.High,
            fdce.getScalarTerm("CLR").getResetActiveLevel(),
        )

        fdse = library.getSNLDesign("FDSE")
        self.assertTrue(fdse.getScalarTerm("S").is_sync_set())

        ram32m = library.getSNLDesign("RAM32M")
        self.assertTrue(ram32m.getScalarTerm("WCLK").is_clock())
        self.assertEqual(
            naja.SNLTermRole.MemoryWriteEnable,
            ram32m.getScalarTerm("WE").getRole(),
        )
        self.assertEqual(
            naja.SNLTermRole.MemoryReadData,
            ram32m.getBusTerm("DOA").getBusTermBit(0).getRole(),
        )

        lut2_ins0 = top.create_child_instance('LUT2', 'ins0')
        lut2_ins1 = top.create_child_instance('LUT2', 'ins1')
        self.assertIsNotNone(lut2_ins0)
        self.assertIsNotNone(lut2_ins1)

        net1 = top.create_net("net1") 
        net2 = top.create_net("net2")

        i.connect_lower_net(net1)
        lut2_ins0.get_term("I0").connect_upper_net(net1)
        lut2_ins0.get_term("O").connect_upper_net(net2)
        lut2_ins1.get_term("I0").connect_upper_net(net2)
        o.connect_lower_net(net2)

        top.dump_full_dot('./test_xilinx_primitives.dot')
        with self.assertRaises(Exception) as context: top.dump_full_dot(-1)
        top.dump_context_dot('./test_xilinx_primitives_context.dot')
        with self.assertRaises(Exception) as context: top.dump_context_dot(-1)

        lut2_ins1.get_term("I0").get_equipotential().dump_dot('./test_xilinx_primitives_lut2_ins1.dot')

        leaf_drivers_count = 0
        for leaf_driver in lut2_ins1.get_term("I0").get_equipotential().get_leaf_drivers():
            leaf_drivers_count += 1
        top_readers_count = 0
        for top_reader in lut2_ins1.get_term("I0").get_equipotential().get_top_readers():
            top_readers_count += 1
        top_drivers_count = 0
        for top_driver in lut2_ins0.get_term("I0").get_equipotential().get_top_drivers():
            top_drivers_count +=1
        
        self.assertEqual(i.get_equipotential(), lut2_ins0.get_term("I0").get_equipotential())

        self.assertEqual(1, leaf_drivers_count)
        self.assertEqual(1, top_readers_count)
        self.assertEqual(1, top_drivers_count)
        self.assertEqual(lut2_ins0.get_design(), top)

        leaf_count = 0
        for leaf in top.get_leaf_children():
            leaf_count += 1
        
        self.assertEqual(2, leaf_count)

        child_instance = top.get_child_instance_by_id(0)
        self.assertIsNotNone(child_instance)
        child_instance.delete()
        lut2_ins1.delete()

    def test_errors(self):
        with self.assertRaises(Exception) as context:
            netlist.load_primitives('unknown')

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
