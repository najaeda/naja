# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import shutil
import unittest
import faulthandler

from najaeda import snl
from najaeda import netlist
from najaeda import instance_visitor

class NajaNetlistTest1(unittest.TestCase):
    def setUp(self):
        universe = snl.SNLUniverse.create()
        db = snl.SNLDB.create(universe)
        universe.setTopDB(db)
        primitives = snl.SNLLibrary.createPrimitives(db)
        and2 = snl.SNLDesign.createPrimitive(primitives, "AND2")
        snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Input, "I0")
        snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Input, "I1")
        snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Output, "O")
        or2 = snl.SNLDesign.createPrimitive(primitives, "OR2")
        snl.SNLScalarTerm.create(or2, snl.SNLTerm.Direction.Input, "I0")
        snl.SNLScalarTerm.create(or2, snl.SNLTerm.Direction.Input, "I1")
        snl.SNLScalarTerm.create(or2, snl.SNLTerm.Direction.Output, "O")
        inv = snl.SNLDesign.createPrimitive(primitives, "INV")
        snl.SNLScalarTerm.create(inv, snl.SNLTerm.Direction.Input, "I")
        snl.SNLScalarTerm.create(inv, snl.SNLTerm.Direction.Output, "O")

        modules = snl.SNLLibrary.create(db, 'Modules')
        module0 = snl.SNLDesign.create(modules, 'Module0')
        i0 = snl.SNLScalarTerm.create(module0, snl.SNLTerm.Direction.Input, 'I0')
        i0Net = snl.SNLScalarNet.create(module0, 'I0')
        i0.setNet(i0Net)
        i1 = snl.SNLScalarTerm.create(module0, snl.SNLTerm.Direction.Input, 'I1')
        i1Net = snl.SNLScalarNet.create(module0, 'I1')
        i1.setNet(i1Net)
        o = snl.SNLScalarTerm.create(module0, snl.SNLTerm.Direction.Output, 'O')
        oNet = snl.SNLScalarNet.create(module0, 'O')
        o.setNet(oNet)
        and0Net = snl.SNLScalarNet.create(module0, 'and0O')
        and1Net = snl.SNLScalarNet.create(module0, 'and1O')
        and0 = snl.SNLInstance.create(module0, and2, 'and0')
        and0.getInstTerm(and0.getModel().getScalarTerm('I0')).setNet(i0Net)
        and0.getInstTerm(and0.getModel().getScalarTerm('I1')).setNet(i1Net)
        and0.getInstTerm(and0.getModel().getScalarTerm('O')).setNet(and0Net)
        and1 = snl.SNLInstance.create(module0, and2, 'and1')
        and1.getInstTerm(and1.getModel().getScalarTerm('I0')).setNet(i0Net)
        and1.getInstTerm(and1.getModel().getScalarTerm('I1')).setNet(i1Net)
        and1.getInstTerm(and1.getModel().getScalarTerm('O')).setNet(and1Net)
        or0 = snl.SNLInstance.create(module0, or2, 'or0')
        or0.getInstTerm(or0.getModel().getScalarTerm('I0')).setNet(and0Net)
        or0.getInstTerm(or0.getModel().getScalarTerm('I1')).setNet(and0Net)
        or0.getInstTerm(or0.getModel().getScalarTerm('O')).setNet(oNet)
        ##create hierarchical netlist
        top = netlist.create_top('Top')
        i0 = top.create_input_bus_term('I0', 1, 0)
        i0Net = top.create_bus_net('I0', 1, 0)
        i0.connect(i0Net)
        i1 = top.create_input_bus_term('I1', 1, 0)
        i1Net = top.create_bus_net('I1', 1, 0)
        i1.connect(i1Net)
        o = top.create_output_term('O')
        oNet = top.create_net('O')
        o.connect(oNet)
        ins0 = top.create_child_instance('Module0', 'Ins0')
        ins0.get_term('I0').connect(i0Net.get_bit(0))
        ins0.get_term('I1').connect(i1Net.get_bit(0))
        ins1 = top.create_child_instance('Module0', 'Ins1')
        ins1.get_term('I0').connect(i0Net.get_bit(1))
        ins1.get_term('I1').connect(i1Net.get_bit(1))
        ins2 = top.create_child_instance('OR2', 'Ins2')
        wire = top.create_bus_net('net', 1, 0)
        ins2.get_term('I0').connect(wire.get_bit(1))
        ins2.get_term('I1').connect(wire.get_bit(0))
        ins2.get_term('O').connect(oNet)

    def tearDown(self):
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()

    def test_browse(self):
        top = netlist.get_top()
        self.assertIsNotNone(top)
        self.assertEqual(top.get_name(), 'Top')
        #DB is the second one created
        # Top is third created module
        self.assertEqual((1,2,0), top.get_model_id())
        self.assertEqual('Top', netlist.get_model_name(top.get_model_id()))
        self.assertIsNone(netlist.get_model_name((1,0,30)))

        self.assertEqual(3, sum(1 for _ in top.get_terms()))
        self.assertEqual(2+2+1, sum(1 for _ in top.get_flat_terms()))
        self.assertEqual(4, sum(1 for _ in top.get_nets()))
        self.assertEqual(2+2+1+2, sum(1 for _ in top.get_flat_nets()))
        self.assertEqual(2, sum(1 for _ in top.get_input_terms()))
        self.assertEqual(2+2, sum(1 for _ in top.get_flat_input_terms()))
        self.assertEqual(1, sum(1 for _ in top.get_output_terms()))
        self.assertEqual(1, sum(1 for _ in top.get_flat_output_terms()))

        i0 = top.get_term('I0')
        self.assertIsNotNone(i0)
        self.assertTrue(i0.is_bus())
        self.assertTrue(i0.is_input())

        i1 = top.get_term('I1')
        self.assertIsNotNone(i1)
        self.assertEqual('I1', i1.get_name())
        self.assertTrue(i1.is_input())
        self.assertTrue(i1.is_bus())
        self.assertFalse(i1.is_scalar())
        self.assertFalse(i1.is_bus_bit())
        self.assertFalse(i1.is_bit())
        self.assertEqual(i1.get_lsb(), 0)
        self.assertEqual(i1.get_msb(), 1)
        self.assertEqual(i1.get_width(), 2)
        self.assertEqual(i1.get_direction(), netlist.Term.INPUT)
        
        o = top.get_term('O')
        self.assertIsNotNone(o)
        self.assertTrue(o.is_output())
        self.assertFalse(o.is_bus())
        self.assertTrue(o.is_scalar())
        self.assertFalse(o.is_bus_bit())
        self.assertTrue(o.is_bit())
        self.assertEqual(o.get_direction(), netlist.Term.OUTPUT)
        self.assertEqual(o.get_name(), 'O')
        self.assertEqual(o.get_width(), 1)
        self.assertIsNone(o.get_lsb())
        self.assertIsNone(o.get_msb())
        self.assertEqual(1, sum(1 for _ in o.get_bits()))
        self.assertEqual(o, next(o.get_bits()))
        self.assertListEqual([i0, i1, o], list(top.get_terms()))

        i0Net = top.get_net('I0')
        self.assertIsNotNone(i0Net)
        self.assertEqual(i0Net.get_name(), 'I0')
        self.assertEqual(i0Net.get_lsb(), 0)
        self.assertEqual(i0Net.get_msb(), 1)
        self.assertEqual(i0Net.get_width(), 2)
        self.assertTrue(i0Net.is_bus())
        self.assertEqual(2, sum(1 for _ in i0Net.get_bits()))
        self.assertIsNone(i0.get_net())
        self.assertIsNotNone(i0.get_lower_net())
        self.assertEqual(i0Net, i0.get_lower_net())

        i1Net = top.get_net('I1')
        self.assertIsNotNone(i1Net)
        self.assertEqual(i1Net.get_name(), 'I1')
        self.assertEqual(i1Net.get_lsb(), 0)
        self.assertEqual(i1Net.get_msb(), 1)
        self.assertEqual(i1Net.get_width(), 2)
        self.assertTrue(i1Net.is_bus())
        self.assertFalse(i1Net.is_scalar())
        self.assertFalse(i1Net.is_bus_bit())
        self.assertFalse(i1Net.is_bit())
        self.assertEqual(2, sum(1 for _ in i1Net.get_bits()))
        self.assertListEqual([i0Net.get_bit(1), i0Net.get_bit(0)], list(i0Net.get_bits()))
        self.assertEqual([i0.get_bit(0)], list(i0Net.get_bit(0).get_terms()))
        self.assertEqual([i0.get_bit(1)], list(i0Net.get_bit(1).get_terms()))
        self.assertIsNone(i1.get_net())
        self.assertIsNotNone(i1.get_lower_net())
        self.assertEqual(i1Net, i1.get_lower_net())

        #self.assertGreater(i1Net, i0Net)
        #self.assertGreaterEqual(i1Net, i0Net)
        #self.assertLess(i0Net, i1Net)
        #self.assertLessEqual(i0Net, i1Net)
        ins0 = top.get_child_instance('Ins0')
        self.assertIsNotNone(ins0)
        self.assertEqual('Ins0', ins0.get_name())
        self.assertEqual('Module0', ins0.get_model_name())
        self.assertFalse(ins0.is_primitive())
        ins1 = top.get_child_instance('Ins1')
        self.assertIsNotNone(ins1)
        self.assertEqual('Ins1', ins1.get_name())
        self.assertFalse(ins1.is_primitive())
        ins2 = top.get_child_instance('Ins2')
        self.assertIsNotNone(ins2)
        self.assertEqual('Ins2', ins2.get_name())
        self.assertTrue(ins2.is_primitive())

        self.assertEqual(3, sum(1 for _ in top.get_child_instances()))
        self.assertListEqual([ins0, ins1, ins2], list(top.get_child_instances()))

        self.assertEqual([ins0.get_term('I0')], list(i0Net.get_bit(0).get_inst_terms()))
        self.assertEqual([i0.get_bit(0), ins0.get_term('I0')], list(i0Net.get_bit(0).get_components()))
        self.assertEqual([i1.get_bit(0), ins0.get_term('I1')], list(i1Net.get_bit(0).get_components()))
        self.assertEqual([ins0.get_term('I1')], list(i1Net.get_bit(0).get_inst_terms()))
        self.assertEqual([i0.get_bit(1), ins1.get_term('I0')], list(i0Net.get_bit(1).get_components()))
        self.assertEqual([i1.get_bit(1), ins1.get_term('I1')], list(i1Net.get_bit(1).get_components()))
        self.assertIsNone(ins0.get_term('I0').get_bit(0))
        self.assertIsNone(ins0.get_term('I0').get_bit(4))
        self.assertEqual(ins0.get_term('I0').get_net(), i0Net.get_bit(0))
        self.assertEqual(ins0.get_term('I1').get_net(), i1Net.get_bit(0))

        oNet = top.get_net('O')
        self.assertIsNotNone(oNet)
        self.assertFalse(oNet.is_bus())
        self.assertTrue(oNet.is_scalar())
        self.assertFalse(oNet.is_bus_bit())
        self.assertTrue(oNet.is_bit())
        self.assertIsNone(oNet.get_msb())
        self.assertIsNone(oNet.get_lsb())
        self.assertEqual(1, sum(1 for _ in oNet.get_bits()))
        self.assertEqual([oNet], list(oNet.get_bits()))
        self.assertIsNone(oNet.get_bit(0))
        self.assertIsNone(o.get_net())
        self.assertIsNotNone(o.get_lower_net())
        self.assertEqual(oNet, o.get_lower_net())

        self.assertIsNone(top.get_child_instance('Ins3'))
        self.assertIsNone(top.get_term('I2'))

    def testInstanceHash(self):
        top = netlist.get_top()
        instances = list(top.get_child_instances())
        self.assertEqual(3, len(instances))
        instancesDict = {}
        for index, instance in enumerate(instances):
            instancesDict[instance] = index
        instancesDict[top] = 4
        self.assertEqual(4, len(instancesDict))
        self.assertEqual(4, instancesDict[top])
        #FIXME xtof can we find back in a dict
        #different but == instances ?
        #self.assertIn(top.get_child_instance('Ins0'), instancesDict)
        #self.assertEqual(0, instancesDict[top.get_child_instance('Ins0')])
        #self.assertEqual(1, instancesDict[top.get_child_instance('Ins1')])
        #self.assertEqual(2, instancesDict[top.get_child_instance('Ins2')])

    def testPrimitiveInstances(self):
        top = netlist.get_top()
        self.assertIsNotNone(top)
        self.assertEqual(3, sum(1 for _ in top.get_child_instances()))
        #self.assertEqual(6, sum(1 for _ in top.get_flat_primitive_instances()))
        #self.assertListEqual(
        #    [top.get_child_instance('Ins0').get_child_instance('and0'),
        #     top.get_child_instance('Ins0').get_child_instance('and1'),
        #     top.get_child_instance('Ins0').get_child_instance('or0'),
        #     top.get_child_instance('Ins1').get_child_instance('and0'),
        #     top.get_child_instance('Ins1').get_child_instance('and1'),
        #     top.get_child_instance('Ins1').get_child_instance('or0'),
        #     top.get_child_instance('Ins2')],
        #    list(top.get_flat_primitive_instances())
        #)
    
    def testDump(self):
        bench_dir = os.environ.get('NAJAEDA_TEST_PATH')
        self.assertIsNotNone(bench_dir)
        bench_dir = os.path.join(bench_dir, "test_najaeda_netlist1")
        if os.path.exists(bench_dir):
            shutil.rmtree(bench_dir)
        os.makedirs(bench_dir)
        top = netlist.get_top()
        top.dump_verilog(os.path.join(bench_dir), "netlist1.v")

    def testInstanceVisitor(self):
        top = netlist.get_top()
        self.assertIsNotNone(top)
        visitor = instance_visitor.Visitor(top)

        # Define the callback to execute at each node
        def callback(instance):
            print(f"Visited instance: {instance}")

        visitor_config = instance_visitor.VisitorConfig(callback=callback)
        visitor.visit(top, visitor_config)
    
if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()