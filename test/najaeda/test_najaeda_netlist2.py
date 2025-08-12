# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import shutil
import unittest
import faulthandler
import logging

from najaeda import netlist
from najaeda import naja

#close to netlist1 test but with more busses

class NajaNetlistTest2(unittest.TestCase):
    def setUp(self):
        logging.basicConfig(level=logging.DEBUG)
        universe = naja.NLUniverse.create()
        db = naja.NLDB.create(universe)
        universe.setTopDB(db)
        primitives = naja.NLLibrary.createPrimitives(db)
        and4 = naja.SNLDesign.createPrimitive(primitives, "AND4")
        naja.SNLScalarTerm.create(and4, naja.SNLTerm.Direction.Input, "I0")
        naja.SNLScalarTerm.create(and4, naja.SNLTerm.Direction.Input, "I1")
        naja.SNLScalarTerm.create(and4, naja.SNLTerm.Direction.Input, "I2")
        naja.SNLScalarTerm.create(and4, naja.SNLTerm.Direction.Input, "I3")
        naja.SNLScalarTerm.create(and4, naja.SNLTerm.Direction.Output, "O")
        inv = naja.SNLDesign.createPrimitive(primitives, "INV")
        naja.SNLScalarTerm.create(inv, naja.SNLTerm.Direction.Input, "I")
        naja.SNLScalarTerm.create(inv, naja.SNLTerm.Direction.Output, "O")

        modules = naja.NLLibrary.create(db, 'Modules')
        module0 = naja.SNLDesign.create(modules, 'Module0')
        i0 = naja.SNLBusTerm.create(module0, naja.SNLTerm.Direction.Input, 3, 0, 'I0')
        i0Net = naja.SNLBusNet.create(module0, 3, 0, 'I0')
        i0.setNet(i0Net)
        i1 = naja.SNLBusTerm.create(module0, naja.SNLTerm.Direction.Input, 3, 0, 'I1')
        i1Net = naja.SNLBusNet.create(module0, 3, 0, 'I1')
        i1.setNet(i1Net)
        o = naja.SNLBusTerm.create(module0, naja.SNLTerm.Direction.Output, 1, 0, 'O')
        oNet = naja.SNLBusNet.create(module0, 1, 0, 'O')
        o.setNet(oNet)
        
        prim = naja.SNLDesign.createPrimitive(primitives, 'Prim')
        naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "O")
        prim.setTruthTables([2,4])
        
        primSeq = naja.SNLDesign.createPrimitive(primitives, 'PrimSeq')
        naja.SNLScalarTerm.create(primSeq, naja.SNLTerm.Direction.Output, "o0")
        naja.SNLScalarTerm.create(primSeq, naja.SNLTerm.Direction.Input, "i0")
        naja.SNLScalarTerm.create(primSeq, naja.SNLTerm.Direction.Input, "c")
        naja.SNLScalarTerm.create(primSeq, naja.SNLTerm.Direction.Input, "i1")
        naja.SNLScalarTerm.create(primSeq, naja.SNLTerm.Direction.Input, "o1")

        net = naja.SNLBusNet.create(module0, 1, 0, 'net')

        and0Net = naja.SNLScalarNet.create(module0, 'and0O')
        and1Net = naja.SNLScalarNet.create(module0, 'and1O')
        and0 = naja.SNLInstance.create(module0, and4, 'and0')
        and0.getInstTerm(and4.getScalarTerm('I0')).setNet(i0Net.getBit(0))
        and0.getInstTerm(and4.getScalarTerm('I1')).setNet(i0Net.getBit(1))
        and0.getInstTerm(and4.getScalarTerm('I2')).setNet(i0Net.getBit(2))
        and0.getInstTerm(and4.getScalarTerm('I3')).setNet(i0Net.getBit(3))
        and0.getInstTerm(and4.getScalarTerm('O')).setNet(net.getBit(0))

        and1 = naja.SNLInstance.create(module0, and4, 'and1')
        and1.getInstTerm(and4.getScalarTerm('I0')).setNet(i1Net.getBit(0))
        and1.getInstTerm(and4.getScalarTerm('I1')).setNet(i1Net.getBit(1))
        and1.getInstTerm(and4.getScalarTerm('I2')).setNet(i1Net.getBit(2))
        and1.getInstTerm(and4.getScalarTerm('I3')).setNet(i1Net.getBit(3))
        and1.getInstTerm(and4.getScalarTerm('O')).setNet(net.getBit(1))
        
        inv0 = naja.SNLInstance.create(module0, inv, 'in0')
        inv0.getInstTerm(inv.getScalarTerm('I')).setNet(net.getBit(0))
        inv0.getInstTerm(inv.getScalarTerm('O')).setNet(oNet.getBit(0))

        inv1 = naja.SNLInstance.create(module0, inv, 'in1')
        inv1.getInstTerm(inv.getScalarTerm('I')).setNet(net.getBit(1))
        inv1.getInstTerm(inv.getScalarTerm('O')).setNet(oNet.getBit(1))

    def tearDown(self):
        netlist.reset()

    def test_top0(self):
        def create_top():
            top = netlist.create_top('Top')
            i0 = top.create_input_bus_term('I0', 3, 0)
            i0Net = top.create_bus_net('I0', 3, 0)
            i0.connect(i0Net)
            i1 = top.create_input_bus_term('I1', 3, 0)
            i1Net = top.create_bus_net('I1', 3, 0)
            i1.connect(i1Net)
            o = top.create_output_bus_term('O', 1, 0)
            oNet = top.create_bus_net('O', 1, 0)
            o.connect(oNet)
            mod = top.create_child_instance('Module0', 'mod')
            mod.get_term('I0').connect(i0Net)
            mod.get_term('I1').connect(i1Net)
            mod.get_term('O').connect(oNet)

        create_top()
        #get nets from terms
        top = netlist.get_top()
        self.assertIsNotNone(top)

        self.assertEqual(1, sum(1 for _ in top.get_output_terms()))
        self.assertEqual(1, top.count_output_terms())
        self.assertEqual(2, sum(1 for _ in top.get_flat_output_terms()))
        self.assertEqual(2, top.count_flat_output_terms())

        mod = top.get_child_instance('mod')
        self.assertIsNotNone(mod)
        self.assertRaises(ValueError, mod.get_child_instance, [])

        modI0 = mod.get_term('I0')
        self.assertIsNotNone(modI0)
        modI0Net = modI0.get_net()
        self.assertIsNotNone(modI0Net)
        self.assertTrue(modI0Net.is_bus())
        self.assertEqual(top.get_net('I0'), modI0Net)

        modI1 = mod.get_term('I1')
        self.assertIsNotNone(modI1)
        modI1Net = modI1.get_net()
        self.assertIsNotNone(modI1Net)
        self.assertTrue(modI1Net.is_bus())
        self.assertEqual(top.get_net('I1'), modI1Net)

        modO = mod.get_term('O')
        self.assertIsNotNone(modO)
        modONet = modO.get_net()
        self.assertIsNotNone(modONet)
        self.assertTrue(modONet.is_bus())
        self.assertEqual(top.get_net('O'), modONet)

        #get inside mod
        modAnd0 = mod.get_child_instance('and0')
        self.assertIsNotNone(modAnd0)
        self.assertEqual(modAnd0, top.get_child_instance(['mod', 'and0']))
        modAnd0I0 = modAnd0.get_term('I0')
        self.assertIsNotNone(modAnd0I0)
        self.assertFalse(modAnd0I0.is_bus())

        #dump top0
        bench_dir = os.environ.get('NAJAEDA_TEST_PATH')
        self.assertIsNotNone(bench_dir)
        bench_dir = os.path.join(bench_dir, "test_najaeda_netlist2_top0")
        if os.path.exists(bench_dir):
            shutil.rmtree(bench_dir)
        os.makedirs(bench_dir)
        top = netlist.get_top()
        top.dump_verilog(os.path.join(bench_dir, "netlist2_top0.v"))
        self.assertRaises(ValueError, top.dump_verilog, "netlist")
        self.assertRaises(FileNotFoundError, top.dump_verilog, os.path.join("non_existing", "netlist2_top0.v"))

        #dump naja if
        netlist.dump_naja_if(os.path.join(bench_dir, "netlist2_top0.najaif"))
        netlist.reset()
        self.assertRaises(FileNotFoundError, netlist.load_naja_if, os.path.join(bench_dir, "non_existing.najaif"))
        #load naja if
        netlist.load_naja_if(os.path.join(bench_dir, "netlist2_top0.najaif"))

    def test_top1(self):
        def create_top():
            top = netlist.create_top('Top')
            i0 = top.create_input_bus_term('I0', 3, 0)
            i0Net = top.create_bus_net('I0_net', 0, 3)
            i0.connect(i0Net)
            i1 = top.create_input_bus_term('I1', 3, 0)
            i1Net = top.create_bus_net('I1_net', 0, 3)
            i1.connect(i1Net)
            o = top.create_output_bus_term('O', 1, 0)
            oNet = top.create_bus_net('O_net', 0, 1)
            o.connect(oNet)
            mod = top.create_child_instance('Module0', 'mod')
            mod.get_term('I0').connect(i0Net)
            mod.get_term('I1').connect(i1Net)
            mod.get_term('O').connect(oNet)

        create_top()
        #get nets from terms
        top = netlist.get_top()
        self.assertIsNotNone(top)
        mod = top.get_child_instance('mod')
        self.assertIsNotNone(mod)

        topI0 = top.get_term('I0')
        self.assertIsNotNone(topI0)
        self.assertEqual(topI0.get_msb(), 3)
        self.assertEqual(topI0.get_lsb(), 0)
        self.assertIsNone(topI0.get_net())
        topI0Net = topI0.get_lower_net()
        self.assertIsNotNone(topI0Net)
        self.assertTrue(topI0Net.is_bus())
        self.assertEqual(top.get_net('I0_net'), topI0Net)
        self.assertEqual(topI0Net.get_msb(), 0)
        self.assertEqual(topI0Net.get_lsb(), 3)

        topI1 = top.get_term('I1')
        self.assertIsNotNone(topI1)
        self.assertEqual(topI1.get_msb(), 3)
        self.assertEqual(topI1.get_lsb(), 0)
        self.assertIsNone(topI1.get_net())
        topI1Net = topI1.get_lower_net()
        self.assertIsNotNone(topI1Net)
        self.assertTrue(topI1Net.is_bus())
        self.assertEqual(top.get_net('I1_net'), topI1Net)
        self.assertEqual(topI1Net.get_msb(), 0)
        self.assertEqual(topI1Net.get_lsb(), 3)

        topO = top.get_term('O')
        self.assertIsNotNone(topO)
        self.assertEqual(topO.get_msb(), 1)
        self.assertEqual(topO.get_lsb(), 0)
        self.assertIsNone(topO.get_net())
        topONet = topO.get_lower_net()
        self.assertIsNotNone(topONet)
        self.assertTrue(topONet.is_bus())
        self.assertEqual(top.get_net('O_net'), topONet)
        self.assertEqual(topONet.get_msb(), 0)
        self.assertEqual(topONet.get_lsb(), 1)

        self.assertEqual(1, sum(1 for _ in top.get_child_instances()))
        mod = top.get_child_instance('mod')
        self.assertIsNotNone(mod)

        modI0 = mod.get_term('I0')
        self.assertTrue(modI0.is_bus())
        self.assertEqual(modI0.get_net(), topI0Net)
        self.assertEqual(modI0.get_msb(), 3)
        self.assertEqual(modI0.get_lsb(), 0)
        self.assertEqual('I0', modI0.get_lower_net().get_name())
        self.assertEqual(modI0.get_lower_net().get_msb(), 3)
        self.assertEqual(modI0.get_lower_net().get_lsb(), 0)
        modI1 = mod.get_term('I1')
        self.assertTrue(modI1.is_bus())
        self.assertEqual(modI1.get_net(), topI1Net)
        self.assertEqual(modI1.get_msb(), 3)
        self.assertEqual(modI1.get_lsb(), 0)
        self.assertEqual('I1', modI1.get_lower_net().get_name())
        self.assertEqual(modI1.get_lower_net().get_msb(), 3)
        self.assertEqual(modI1.get_lower_net().get_lsb(), 0)
        modO = mod.get_term('O')
        self.assertTrue(modO.is_bus())
        self.assertEqual(modO.get_net(), topONet)
        self.assertEqual(modO.get_msb(), 1)
        self.assertEqual(modO.get_lsb(), 0)
        self.assertEqual('O', modO.get_lower_net().get_name())
        self.assertEqual(modO.get_lower_net().get_msb(), 1)
        self.assertEqual(modO.get_lower_net().get_lsb(), 0)

        # dump top1
        bench_dir = os.environ.get('NAJAEDA_TEST_PATH')
        self.assertIsNotNone(bench_dir)
        bench_dir = os.path.join(bench_dir, "test_najaeda_netlist2_top1")
        if os.path.exists(bench_dir):
            shutil.rmtree(bench_dir)
        os.makedirs(bench_dir)
        top = netlist.get_top()
        top.dump_verilog(os.path.join(bench_dir, "netlist2_top1.v"))

    def test_top2(self):
        def create_top():
            top = netlist.create_top('Top')
            i0 = top.create_input_bus_term('I0', 3, 0)
            i1 = top.create_input_bus_term('I1', 3, 0)
            o = top.create_output_bus_term('O', 1, 0)
            mod = top.create_child_instance('Module0', 'mod')
            for i in range(4):
                i0Net = top.create_net(f'I0_{i}')
                i0.get_bit(i).connect(i0Net)
                mod.get_term('I0').get_bit(i).connect(i0Net)
            for i in range(4):
                i1Net = top.create_net(f'I1_{3-i}')
                i1.get_bit(i).connect(i1Net)
                mod.get_term('I1').get_bit(i).connect(i1Net)
            oNet = top.create_bus_net('O_net', 0, 1)
            o.connect(oNet)

        create_top()
        top = netlist.get_top()
        self.assertIsNotNone(top)
        mod = top.get_child_instance('mod')
        self.assertIsNotNone(mod)

        modI0 = mod.get_term('I0')
        self.assertTrue(modI0.is_bus())
        self.assertEqual(modI0.get_msb(), 3)
        self.assertEqual(modI0.get_lsb(), 0)
        modI0Net = modI0.get_net()
        self.assertIsNotNone(modI0Net)
        self.assertEqual(modI0Net.get_name(), '{I0_3,I0_2,I0_1,I0_0}')
        self.assertEqual(modI0Net.get_width(), 4)
        self.assertIsNone(modI0Net.get_msb())
        self.assertIsNone(modI0Net.get_lsb())
        self.assertFalse(modI0Net.is_bus())
        self.assertFalse(modI0Net.is_scalar())
        self.assertTrue(modI0Net.is_concat())
        self.assertFalse(modI0Net.is_const())
        self.assertEqual(4, sum(1 for _ in modI0Net.get_bits()))
        self.assertIsNone(modI0Net.get_bit(4))
        modI0Net.set_type(netlist.Net.Type.ASSIGN0)
        self.assertTrue(modI0Net.is_const())

        for i in range(4):
            # modIO.get_bit(i) is the connected to I0_{3-i} scalar net
            self.assertEqual(modI0Net.get_bit(i).get_name(), top.get_net(f'I0_{3-i}').get_name())

        modI1 = mod.get_term('I1')
        self.assertTrue(modI1.is_bus())
        self.assertEqual(modI1.get_msb(), 3)
        self.assertEqual(modI1.get_lsb(), 0)
        modI1Net = modI1.get_net()
        self.assertIsNotNone(modI1Net)
        self.assertEqual(modI1Net.get_name(), '{I1_0,I1_1,I1_2,I1_3}')
        self.assertEqual(modI1Net.get_width(), 4)
        self.assertIsNone(modI1Net.get_msb())
        self.assertIsNone(modI1Net.get_lsb())
        self.assertFalse(modI1Net.is_bus())
        self.assertFalse(modI1Net.is_scalar())
        self.assertTrue(modI1Net.is_concat())
        for i in range(4):
            # modI1.get_bit(i) is the connected to I0_{i} scalar net
            self.assertEqual(modI1Net.get_bit(i).get_name(), top.get_net(f'I1_{i}').get_name())

        topO = top.get_term('O')
        self.assertEqual(topO.get_msb(), 1)
        self.assertEqual(topO.get_lsb(), 0)
        topOLowerNet = topO.get_lower_net()
        self.assertIsNotNone(topOLowerNet)
        self.assertTrue(topOLowerNet.is_bus())
        self.assertEqual(topOLowerNet.get_msb(), 0)
        self.assertEqual(topOLowerNet.get_lsb(), 1)
        self.assertEqual(topO.get_bit(0).get_lower_net(), topOLowerNet.get_bit(1))
        self.assertEqual(topO.get_bit(1).get_lower_net(), topOLowerNet.get_bit(0))
        self.assertEqual(top.get_net('O_net'), topOLowerNet)
        self.assertFalse(topOLowerNet.is_concat())
        self.assertFalse(topOLowerNet.is_const())
        self.assertEqual(2, sum(1 for _ in topOLowerNet.get_bits()))
        topOLowerNet.set_type(netlist.Net.Type.ASSIGN1)
        self.assertTrue(topOLowerNet.is_const())
    
        # dump top2
        bench_dir = os.environ.get('NAJAEDA_TEST_PATH')
        self.assertIsNotNone(bench_dir)
        bench_dir = os.path.join(bench_dir, "test_najaeda_netlist2_top2")
        if os.path.exists(bench_dir):
            shutil.rmtree(bench_dir)
        os.makedirs(bench_dir)
        top = netlist.get_top()
        top.dump_verilog(os.path.join(bench_dir, "netlist2_top2.v"))

    def test_top3(self):
        def create_top():
            top = netlist.create_top('Top')
            i1 = top.create_input_bus_term('I1', 3, 0)
            o = top.create_inout_bus_term('O', 1, 0)
            mod = top.create_child_instance('Module0', 'mod')

            i0Net = top.create_bus_net('I0', 3, 0)
            for i in range(4):
                i0Net = top.create_net(f'I0_{3-i}')
                mod.get_term('I0').get_bit(i).connect(i0Net)
            for i in range(4):
                i1Net = top.create_net(f'I1_{3-i}')
                i1.get_bit(i).connect(i1Net)
                mod.get_term('I1').get_bit(i).connect(i1Net)
            oNet = top.create_bus_net('O', 1, 0)
            o.connect(oNet)

        create_top()
        top = netlist.get_top()
        self.assertIsNotNone(top)
        mod = top.get_child_instance('mod')
        self.assertIsNotNone(mod)

        # dump top3
        bench_dir = os.environ.get('NAJAEDA_TEST_PATH')
        self.assertIsNotNone(bench_dir)
        bench_dir = os.path.join(bench_dir, "test_najaeda_netlist2_top3")
        if os.path.exists(bench_dir):
            shutil.rmtree(bench_dir)
        os.makedirs(bench_dir)
        top = netlist.get_top()
        top.dump_verilog(os.path.join(bench_dir, "netlist2_top3.v"))
        netlist.apply_constant_propagation()
        netlist.apply_dle()
        top.dump_full_dot("./netlist2_top3.dot")
    
    def testClockRelatedInputsAndOutputs(self):
        top = netlist.create_top('Top')
        inst = top.create_child_instance('PrimSeq', 'inst')
        # Create clock input
        clk = inst.get_term('c')
        self.assertIsNotNone(clk)
        # create clock related input
        in0 = inst.get_term('i0')
        self.assertIsNotNone(in0)
        inst.add_clock_related_inputs(clk, [in0])
        # create clock related output
        out0 = inst.get_term('o0')
        inst.add_clock_related_outputs(clk, [out0])
        self.assertEqual([in0], inst.get_clock_related_inputs(clk))
        self.assertEqual([out0], inst.get_clock_related_outputs(clk))
        inst.add_combinatorial_arcs([inst.get_term('i1')], [inst.get_term('o1')])
        self.assertEqual([inst.get_term('i1')], inst.get_combinatorial_inputs(inst.get_term('o1')))
        self.assertEqual([inst.get_term('o1')], inst.get_combinatorial_outputs(inst.get_term('i1')))
        self.assertTrue(inst.get_term('i0').is_sequential())
        self.assertTrue(inst.get_term('o0').is_sequential())
        self.assertTrue(inst.get_term('c').is_sequential())
        self.assertFalse(inst.get_term('i1').is_sequential())
        self.assertFalse(inst.get_term('o1').is_sequential())
    
    def testMerticsOnNone(self):
        netlist.reset()
        self.assertEqual(0, netlist.get_max_logic_level())
        self.assertEqual(0, netlist.get_max_fanout())
        universe = naja.NLUniverse.create()
        self.assertEqual(0, netlist.get_max_logic_level())
        self.assertEqual(0, netlist.get_max_fanout())
        

        
if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
