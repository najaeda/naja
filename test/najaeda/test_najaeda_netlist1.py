# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import shutil
import unittest
import faulthandler
from najaeda import netlist
from najaeda import snl

class NajaNetlistTest1(unittest.TestCase):
    def setUp(self):
        universe = snl.SNLUniverse.create()
        db = snl.SNLDB.create(universe)
        primitives = snl.SNLLibrary.createPrimitives(db)
        and2 = snl.SNLDesign.createPrimitive(primitives, "AND2")
        i0 = snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Input, "I0")
        i1 = snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Input, "I1")
        o = snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Output, "O")
        inv = snl.SNLDesign.createPrimitive(primitives, "INV")
        i = snl.SNLScalarTerm.create(inv, snl.SNLTerm.Direction.Input, "I")
        o = snl.SNLScalarTerm.create(inv, snl.SNLTerm.Direction.Output, "O")

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
        or0 = snl.SNLInstance.create(module0, and2, 'or0')
        or0.getInstTerm(or0.getModel().getScalarTerm('I0')).setNet(and0Net)
        or0.getInstTerm(or0.getModel().getScalarTerm('I1')).setNet(and0Net)
        or0.getInstTerm(or0.getModel().getScalarTerm('O')).setNet(oNet)
        ##create hierarchical netlist
        top = netlist.create_top('Top')
        busTerm = top.create_input_bus_term('I0', 1, 0)
        top.create_input_bus_term('I1', 1, 0)
        self.assertEqual(busTerm.get_lsb(), 0)
        self.assertEqual(busTerm.get_msb(), 1)
        print(busTerm.get_name())
        top.create_output_term('O')
        ins0 = top.create_child_instance('Module0', 'Ins0')
        ins1 = top.create_child_instance('Module0', 'Ins1')

    def tearDown(self):
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()

    def test_browse(self):
        top = netlist.get_top()
        self.assertIsNotNone(top)
        self.assertEqual(top.get_name(), 'Top')
        #DB is the second one created
        self.assertEqual((2,0,0), top.get_model_id())
        self.assertEqual('Top', netlist.get_model_name(top.get_model_id()))
        self.assertIsNone(netlist.get_model_name((2,0,30)))
    
    def testDump(self):
        bench_dir = os.environ.get('NAJAEDA_TEST_PATH')
        self.assertIsNotNone(bench_dir)
        bench_dir = os.path.join(bench_dir, "test_najaeda_netlist1")
        if os.path.exists(bench_dir):
            shutil.rmtree(bench_dir)
        os.makedirs(bench_dir)
        top = netlist.get_top()
        top.dump_verilog(os.path.join(bench_dir), "cloned.v")
    
if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()