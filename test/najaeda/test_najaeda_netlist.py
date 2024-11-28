# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import faulthandler
from najaeda import netlist
from najaeda import snl

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

class NajaNetlistTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()

    def test_loader(self):
        design_files = [os.path.join(verilog_benchmarks, "test0.v")]
        primitives = [os.path.join(liberty_benchmarks, "asap7_excerpt" , "test0.lib")]
        netlist.load_liberty(primitives)
        netlist.load_verilog(design_files)
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()

    def test_instance(self):
        u = snl.SNLUniverse.create()
        db = snl.SNLDB.create(u)
        lib = snl.SNLLibrary.create(db)
        self.top = snl.SNLDesign.create(lib)
        u.setTopDesign(self.top)
        self.model = snl.SNLDesign.create(lib)
        self.submodel = snl.SNLDesign.create(lib, "submodel")
        self.i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "I0")
        self.i1 = snl.SNLBusTerm.create(self.model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
        self.o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "O")
        ins2 = snl.SNLInstance.create(self.model, self.submodel, "ins2")
        ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")
        path1 = snl.SNLPath(ins1)
        path2 = snl.SNLPath(path1, ins2)
        instance = netlist.Instance(path1, ins1)
        self.assertEqual(instance.path, path1)
        self.assertEqual(instance.inst, ins1)
        index = 0
        terms_list = []
        for term in instance.get_inst_terms():
            terms_list.append(term)
        for term in instance.get_inst_terms():
            self.assertTrue(term == terms_list[index])
            index += 1
        name_list = ["ins1", "ins2"]
        instance2 = netlist.get_instance_by_path(name_list)
        self.assertEqual(instance2.path, path2)
        self.assertEqual(instance2.inst, ins2)
        print(instance2.path)
        print(instance2.inst)
        print(instance.get_child_instance(ins2.getName()).path)
        print(instance.get_child_instance(ins2.getName()).inst)
        self.assertTrue(instance2.inst == instance.get_child_instance(ins2.getName()).inst)
        self.assertTrue(instance2.path == instance.get_child_instance(ins2.getName()).path)
        self.assertTrue(instance2 == instance.get_child_instance(ins2.getName()))

        self.assertTrue(instance.get_number_of_child_instances() == 1)
        instance.delete_instance(instance2.get_name())
        self.assertTrue(instance.get_number_of_child_instances() == 0)

        instance.create_child_instance(self.submodel, "ins2")
        self.assertTrue(instance.get_number_of_child_instances() == 1)

        #Test bus term creation connection and disconnection
        instance3 = instance.create_child_instance(self.submodel, "ins3")
        self.assertTrue(instance.get_number_of_child_instances() == 2)
        instance3.create_output_bus_term("I1", 4, 0)
        instance.create_bus_net("netI1", 4, 0)
        #connect the bus term to the bus net
        termsForBus = instance3.get_term_list_for_bus("I1")
        self.assertTrue(len(termsForBus) == 5)
        netBitsForBus = instance.get_net_list_for_bus("netI1")
        self.assertTrue(len(netBitsForBus) == 5)
        for i in range(4):
            termsForBus[i].connect(netBitsForBus[i])



    def test_equipotential(self):
        universe = snl.SNLUniverse.create()
        db = snl.SNLDB.create(universe)
        lib = snl.SNLLibrary.create(db)
        self.primitives = snl.SNLLibrary.createPrimitives(db)
        self.top = snl.SNLDesign.create(lib)
        universe.setTopDesign(self.top)
        self.model = snl.SNLDesign.create(lib, "model")
        self.submodel = snl.SNLDesign.createPrimitive(self.primitives, "submodel")
        self.i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "I0")
        self.i1 = snl.SNLBusTerm.create(self.model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
        self.o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "O")
        self.i0sub = snl.SNLScalarTerm.create(self.submodel, snl.SNLTerm.Direction.Input, "I0")
        self.i1sub = snl.SNLBusTerm.create(self.submodel, snl.SNLTerm.Direction.Input, 4, 0, "I1")
        self.osub = snl.SNLScalarTerm.create(self.submodel, snl.SNLTerm.Direction.Output, "O")
        ins2 = snl.SNLInstance.create(self.model, self.submodel, "ins2")
        ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")

        path0 = snl.SNLPath()
        print(path0)
        self.assertIsNotNone(path0)
        self.assertTrue(path0.empty())
        self.assertEqual(0, path0.size())
        self.assertEqual(snl.SNLPath(), path0.getHeadPath())

        inst_terms = tuple(ins1.getInstTerms())
        i0_net = snl.SNLScalarNet.create(self.top, "I0")
        inst_terms[0].setNet(i0_net)
        i0_net_sub = snl.SNLScalarNet.create(self.model, "I0")
        sub_inst_terms = tuple(ins2.getInstTerms())
        sub_inst_terms[0].setNet(i0_net_sub)
        self.i0.setNet(i0_net_sub)
        print(inst_terms[0])
        
        path1 = snl.SNLPath(path0, ins1)
        self.assertTrue(path1.size() == 1)
        path2 = snl.SNLPath(path1, ins2)
        self.assertTrue(path2.size() == 2)
        self.assertTrue(path1.getHeadInstance() == ins1)
        self.assertTrue(path2.getHeadInstance() == ins1)
        print(path2)
        inst_term = netlist.InstTerm(path2, sub_inst_terms[0])
        equi = netlist.Equipotential(inst_term)
        net_component_occurrence2 = snl.SNLNetComponentOccurrence(path1, sub_inst_terms[0])
        snlequi = snl.SNLEquipotential(net_component_occurrence2)

        snl_top_terms = []
        for t in snlequi.getTerms():
            snl_top_terms.append(t)

        for t in equi.get_top_terms():
            self.assertTrue(t.term == snl_top_terms.pop(0))

        snl_inst_term_occurrences = []
        for t in snlequi.getInstTermOccurrences():
            snl_inst_term_occurrences.append(t)

        for t in equi.get_inst_terms():
            to_compare_with = snl_inst_term_occurrences.pop(0)
            self.assertTrue(t.term == to_compare_with.getInstTerm())
            self.assertTrue(t.path.getHeadPath() == to_compare_with.getPath())

        instance = netlist.Instance(path1, ins1)
        print(instance.get_inst_term("I0").get_net())
        self.assertIsNotNone(instance.get_inst_term("I0"))
        print(netlist.Net(path0, i0_net))
        self.assertTrue(instance.get_inst_term("I0").get_net() == netlist.Net(path1, i0_net))
        print(str(instance.get_inst_term("I0")))
        instance.get_inst_term("I0").disconnect()
        self.assertIsNone(instance.get_inst_term("I0").get_net().net)
        instance.get_inst_term("I0").connect(netlist.Net(path0, i0_net))
        self.assertTrue(instance.get_inst_term("I0").get_net() == netlist.Net(path1, i0_net))

        netlistNet1 = netlist.Net(path1, i0_net)
        netlistNet2 = netlist.Net(path2, i0_net_sub)
        self.assertTrue(netlistNet1 != netlistNet2)
        self.assertTrue(netlistNet2 == netlistNet2)
        self.assertTrue(netlistNet1 == netlist.Net(path1, i0_net))
        self.assertTrue(netlistNet2 == netlist.Net(path2, i0_net_sub))
        self.assertTrue(netlistNet1 < netlistNet2)
        self.assertTrue(netlistNet1 <= netlistNet2)
        self.assertTrue(netlistNet2 > netlistNet1)
        self.assertTrue(netlistNet2 >= netlistNet1)
        print(netlistNet1)    

    def testTopTerm(self):
        universe = snl.SNLUniverse.create()
        db = snl.SNLDB.create(universe)
        lib = snl.SNLLibrary.create(db)
        self.primitives = snl.SNLLibrary.createPrimitives(db)
        self.top = snl.SNLDesign.create(lib)
        universe.setTopDesign(self.top)
        self.i0 = snl.SNLScalarTerm.create(self.top, snl.SNLTerm.Direction.Input, "I0")
        self.i1 = snl.SNLScalarTerm.create(self.top, snl.SNLTerm.Direction.Input, "I1")

        top_term = netlist.TopTerm(self.i0)
        top_term2 = netlist.TopTerm(self.i1)

        self.assertTrue(top_term == top_term)
        self.assertTrue(top_term != top_term2)
        self.assertTrue(top_term < top_term2)
        self.assertTrue(top_term <= top_term2)
        self.assertTrue(top_term2 > top_term)
        self.assertTrue(top_term2 >= top_term)
        print(top_term)
    
    def testInstTerm(self):
        universe = snl.SNLUniverse.create()
        db = snl.SNLDB.create(universe)
        lib = snl.SNLLibrary.create(db)
        self.primitives = snl.SNLLibrary.createPrimitives(db)
        self.top = snl.SNLDesign.create(lib)
        universe.setTopDesign(self.top)
        self.model = snl.SNLDesign.create(lib, "model")
        self.submodel = snl.SNLDesign.createPrimitive(self.primitives, "submodel")
        self.i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "I0")
        self.i1 = snl.SNLBusTerm.create(self.model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
        self.o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "O")
        self.i0sub = snl.SNLScalarTerm.create(self.submodel, snl.SNLTerm.Direction.Input, "I0")
        self.i1sub = snl.SNLBusTerm.create(self.submodel, snl.SNLTerm.Direction.Input, 4, 0, "I1")
        self.osub = snl.SNLScalarTerm.create(self.submodel, snl.SNLTerm.Direction.Output, "O")
        ins2 = snl.SNLInstance.create(self.model, self.submodel, "ins2")
        ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")

        path0 = snl.SNLPath()
        path1 = snl.SNLPath(path0, ins1)
        path2 = snl.SNLPath(path1, ins2)
        instance = netlist.Instance(path1, ins1)
        instTerm0 = instance.get_inst_term("I0")
        instTerm1 = instance.get_inst_term("I1")
        instance2 = netlist.Instance(path2, ins2)
        inst2Term0 = instance2.get_inst_term("I0")
        inst2Term1 = instance2.get_inst_term("I1")

        self.assertTrue(instTerm0 == instTerm0)
        self.assertTrue(instTerm0 != instTerm1)
        self.assertTrue(instTerm0 < instTerm1)
        self.assertTrue(instTerm0 <= instTerm1)
        self.assertTrue(instTerm1 > instTerm0)
        self.assertTrue(instTerm1 >= instTerm0)
        self.assertTrue(instTerm0.get_instance().inst == instance.inst)
        count = 0
        for to in instTerm1.get_flat_fanout():
            count += 1
        self.assertTrue(count == 0)
        self.assertTrue(instTerm0.get_equipotential() == netlist.Equipotential(instTerm0))
        self.assertTrue(instTerm0.is_input())
        self.assertFalse(instTerm0.is_output())





if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
