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

class ComputeEqui:

    def __init__(self, term):
        self.term = term
        self.nets = []
        self.terms = []

    def collect(self, termToCollect):
      #if net of term is in self.nets return
      if termToCollect.get_net() in self.nets:
          return
      #add net of term to self.nets
      self.nets.append(termToCollect.get_net())
      net = termToCollect.get_net()
      for term in net.get_terms():
          if (term.get_instance().is_top()):
                if not (term in self.terms):
                    self.terms.append(term)
                continue      
          if term.get_instance().is_leaf():
                if not (term in self.terms):
                    self.terms.append(term)
                continue
          else:
                for lowerNetTerm in term.get_lower_net().get_terms():
                    self.collect(lowerNetTerm)
    
    def get_terms(self):
        return self.terms


class NajaNetlistTest0(unittest.TestCase):
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
        #print(netlist.get_top())
        #for inst in netlist.get_all_primitive_instances():
        #    print(inst)
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()

    def test_loader1(self):
        design_files = [os.path.join(verilog_benchmarks, "test1.v")]
        lut4 = snl.SNLDesign.createPrimitive(netlist.get_primitives_library(), "LUT4")
        i0 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I0")
        i1 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I1")
        i2 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I2")
        i3 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I3")
        q = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Output, "Q")
        snl.SNLParameter.create_binary(lut4, "INIT", 16, 0x0000)
        netlist.load_verilog(design_files)
        #for inst in netlist.get_all_primitive_instances():
        #    print(inst)
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
        instance = netlist.Instance(path1)
        self.assertEqual(instance.path, path1)
        self.assertEqual(instance.get_model_name(), ins1.getModel().getName())
        index = 0
        terms_list = []
        for term in instance.get_terms():
            terms_list.append(term)
        for term in instance.get_terms():
            self.assertTrue(term == terms_list[index])
            index += 1
        name_list = ["ins1", "ins2"]
        instance2 = netlist.get_instance_by_path(name_list)
        self.assertEqual(instance2.path, path2)
        self.assertEqual(instance2.get_model_name(), ins2.getModel().getName())
        #print(instance2.path)
        #print(instance2.get_model())
        #print(instance.get_child_instance(ins2.getName()).path)
        #print(instance.get_child_instance(ins2.getName()).get_model())
        self.assertEqual(instance2.get_model_name(), instance.get_child_instance(ins2.getName()).get_model_name())
        self.assertEqual(instance2.path, instance.get_child_instance(ins2.getName()).path)
        self.assertEqual(instance2, instance.get_child_instance(ins2.getName()))

        self.assertEqual(instance.get_number_of_child_instances(), 1)
        instance.delete_instance(instance2.get_name())
        self.assertEqual(instance.get_number_of_child_instances(), 0)

        instance.create_child_instance(self.submodel.getName(), "ins2")
        self.assertEqual(instance.get_number_of_child_instances(), 1)
        self.assertIsNotNone(instance.get_child_instance("ins2"))
        self.assertEqual(instance.get_child_instance("ins2").get_name(), "ins2")
        self.assertEqual(instance.get_child_instance("ins2").get_model_name(), self.submodel.getName())

        #Test bus term creation connection and disconnection
        instance3 = instance.create_child_instance(self.submodel.getName(), "ins3")
        self.assertTrue(instance.get_number_of_child_instances() == 2)
        instance3.create_output_bus_term("O1", 4, 0)
        instance.create_bus_net("netO1", 4, 0)
        #connect the bus term to the bus net
        i1 = instance3.get_term("O1")
        self.assertIsNotNone(i1)
        self.assertTrue(i1.is_bus())
        self.assertEqual(i1.get_width(), 5)
        net_i1 = instance.get_net("netO1")
        self.assertEqual(net_i1.get_width(), 5)
        i1.connect(net_i1)

        for flat_output in instance.get_flat_output_terms():
            self.assertEqual(flat_output.get_instance(), instance)
        
        for flat_net in instance.get_flat_nets():
            self.assertEqual(flat_net.path, instance.path)

        # for term in net_i1.get_terms():
        #     self.assertEqual(term.get_net(), net_i1)
        
        # for insterm in net_i1.get_inst_terms():
        #     self.assertEqual(insterm.getNet(), net_i1)
        
        inputCount = 0
        for bit in instance.get_flat_input_terms():
            #print(bit)
            self.assertTrue(bit.is_input())
            self.assertFalse(bit.is_output())
            inputCount += 1
        
        self.assertEqual(inputCount, 6)
        
        outputCount = 0
        for output in instance.get_output_terms():
            self.assertTrue(output.is_output())
            self.assertFalse(output.is_input())
            outputCount += 1
        
        instance.create_input_term("I3")
        instance.create_input_bus_term("I4", 4, -1)
        instance.create_output_term("O2")
        
        inputCount = 0
        for input in instance.get_flat_input_terms():
            self.assertTrue(input.is_input())
            self.assertFalse(input.is_output())
            inputCount += 1
        
        self.assertEqual(inputCount, 13)
        
        outputCount = 0
        for output in instance.get_output_terms():
            self.assertTrue(output.is_output())
            self.assertFalse(output.is_input())
            self.assertTrue(output.get_direction() == snl.SNLTerm.Direction.Output)
            outputCount += 1

        self.assertEqual(outputCount, 2)

        self.assertIsNone(instance.get_net("created_net"))
        instance.create_net("created_net")
        self.assertIsNotNone(instance.get_net("created_net"))

        #print(instance.get_name())

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
        self.i0Top = snl.SNLScalarTerm.create(self.top, snl.SNLTerm.Direction.Input, "I0")

        path0 = snl.SNLPath()
        #print(path0)
        self.assertIsNotNone(path0)
        self.assertTrue(path0.empty())
        self.assertEqual(0, path0.size())
        self.assertEqual(snl.SNLPath(), path0.getHeadPath())

        inst_terms = tuple(ins1.getInstTerms())
        i0_net = snl.SNLScalarNet.create(self.top, "I0")
        inst_terms[0].setNet(i0_net)
        self.i0Top.setNet(i0_net)
        i0_net_sub = snl.SNLScalarNet.create(self.model, "I0")
        sub_inst_terms = tuple(ins2.getInstTerms())
        sub_inst_terms[0].setNet(i0_net_sub)
        self.i0.setNet(i0_net_sub)
        #print(inst_terms[0])
        
        path1 = snl.SNLPath(path0, ins1)
        self.assertTrue(path1.size() == 1)
        path2 = snl.SNLPath(path1, ins2)
        self.assertTrue(path2.size() == 2)
        self.assertTrue(path1.getHeadInstance() == ins1)
        self.assertTrue(path2.getHeadInstance() == ins1)
        #print(path2)
        inst_term = netlist.Term(path2, sub_inst_terms[0].getBitTerm())
        equi = netlist.Equipotential(inst_term)
        net_component_occurrence2 = snl.SNLNetComponentOccurrence(path1, sub_inst_terms[0])
        snlequi = snl.SNLEquipotential(net_component_occurrence2)

        computeEqui = ComputeEqui(inst_term)
        computeEqui.collect(inst_term)
        for term in computeEqui.get_terms():
            print("--", term)

        snl_top_terms = []
        for t in snlequi.getTerms():
            snl_top_terms.append(t)
            print("----",t)

        snl_inst_term_occurrences = []
        for t in snlequi.getInstTermOccurrences():
            snl_inst_term_occurrences.append(t)
            print("----",t)

        
        
        self.assertEqual(len(computeEqui.get_terms()), len(snl_top_terms) + len(snl_inst_term_occurrences))

        for t in equi.get_top_terms():
            self.assertTrue(t.term == snl_top_terms.pop(0)) 
        
        for t in equi.get_inst_terms():
            to_compare_with = snl_inst_term_occurrences.pop(0)
            self.assertTrue(t.term == to_compare_with.getInstTerm().getBitTerm())
            self.assertTrue(t.path.getHeadPath() == to_compare_with.getPath())

        instance = netlist.Instance(path1)
        #for child in instance.get_child_instances():
        #    print(child)
        #print(instance.get_term("I0").get_net())
        self.assertIsNotNone(instance.get_term("I0"))
        #print(netlist.Net(path0, i0_net))
        self.assertEqual(instance.get_term("I0").get_net(), netlist.Net(path1.getHeadPath(), i0_net))
        #print(str(instance.get_term("I0")))
        instance.get_term("I0").disconnect()
        self.assertIsNone(instance.get_term("I0").get_net())
        instance.get_term("I0").connect(netlist.Net(path0, i0_net))
        flat_fanout = 0
        for fanout in instance.get_term("I0").get_flat_fanout():
            flat_fanout += 1
        self.assertEqual(flat_fanout, 1)

        netlistNet1 = netlist.Net(path1, i0_net)
        netlistNet2 = netlist.Net(path2, i0_net_sub)
        self.assertNotEqual(netlistNet1, netlistNet2)
        self.assertEqual(netlistNet2, netlistNet2)
        self.assertEqual(netlistNet1, netlist.Net(path1, i0_net))
        self.assertEqual(netlistNet2, netlist.Net(path2, i0_net_sub))
        #self.assertLess(netlistNet1, netlistNet2)
        #self.assertLessEqual(netlistNet1, netlistNet2)
        #self.assertGreater(netlistNet2, netlistNet1)
        #self.assertGreaterEqual(netlistNet2, netlistNet1)

        #for net in instance.get_nets():
        #    print(net)
        #for flat_net in instance.get_flat_nets():
        #    print(flat_net)
        #for inputterm in instance.get_input_terms():
        #    print(inputterm)
        
        # Validate is quary function in instance

        self.assertFalse(instance.is_assign())
        self.assertFalse(instance.is_blackbox())
        self.assertFalse(instance.is_const0())
        self.assertFalse(instance.is_const1())
        self.assertFalse(instance.is_const())
        self.assertFalse(instance.is_buf())
        self.assertFalse(instance.is_inv())
        self.assertFalse(instance.is_primitive())
        self.assertFalse(instance.is_leaf())

        leaf_count = 0
        for leaf in netlist.get_top().get_leaf_children():
            leaf_count += 1
        self.assertEqual(1, leaf_count)

    def testTopTerm(self):
        universe = snl.SNLUniverse.create()
        db = snl.SNLDB.create(universe)
        lib = snl.SNLLibrary.create(db)
        self.primitives = snl.SNLLibrary.createPrimitives(db)
        self.top = snl.SNLDesign.create(lib)
        universe.setTopDesign(self.top)
        self.i0 = snl.SNLScalarTerm.create(self.top, snl.SNLTerm.Direction.Input, "I0")
        self.i1 = snl.SNLScalarTerm.create(self.top, snl.SNLTerm.Direction.Input, "I1")

        top_term = netlist.Term(snl.SNLPath(), self.i0)
        top_term2 = netlist.Term(snl.SNLPath(), self.i1)

        self.assertEqual(top_term, top_term)
        self.assertNotEqual(top_term, top_term2)
        self.assertLess(top_term, top_term2)
        self.assertLessEqual(top_term, top_term2)
        self.assertGreater(top_term2, top_term)
        self.assertGreaterEqual(top_term2, top_term)
    
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
        instance = netlist.Instance(path1)
        instTerm0 = instance.get_term("I0")
        self.assertIsNotNone(instTerm0)
        instTerm1 = instance.get_term("I1")
        self.assertIsNotNone(instTerm1)
        instance2 = netlist.Instance(path2)
        inst2Term0 = instance2.get_term("I0")
        self.assertIsNotNone(inst2Term0)
        inst2Term1 = instance2.get_term("I1")
        self.assertIsNotNone(inst2Term1)

        self.assertEqual(instTerm0, instTerm0)
        self.assertNotEqual(instTerm0, instTerm1)
        # TODO: Fix the following tests
        #self.assertTrue(instTerm0 < instTerm1)
        #self.assertTrue(instTerm0 <= instTerm1)
        #self.assertTrue(instTerm1 > instTerm0)
        #self.assertTrue(instTerm1 >= instTerm0)
        self.assertEqual(instTerm0.get_instance(), instance)
        self.assertEqual(instTerm0.get_equipotential(), netlist.Equipotential(instTerm0))
        self.assertTrue(instTerm0.is_input())
        self.assertFalse(instTerm0.is_output())

        count = 0
        for bit in instTerm1.get_bits():
            for to in bit.get_flat_fanout():
                count += 1
            self.assertEqual(count, 0)


    def testTop(self):
        netlist.create_top('Top')
        top = netlist.get_top()
        self.assertIsNotNone(top)
        self.assertEqual(top, netlist.get_top())
        self.assertEqual(top.get_name(), "Top")
        top.create_input_term("I0")
        top.create_input_bus_term("I1", 4, 0)
        top.create_output_term("O")
        count = 0
        for input in top.get_flat_input_terms():
            count += 1
        self.assertEqual(count, 6)
        count = 0
        for output in top.get_flat_output_terms():
            count += 1
        self.assertEqual(count, 1)
        top_i1 = top.get_term("I1")
        self.assertIsNotNone(top_i1)
        self.assertTrue(top_i1.is_input())
        self.assertTrue(top_i1.is_bus())
        #self.assertEqual(len(busList), 5)
        top.create_net("netI1") 
        self.assertIsNotNone(top.get_net("netI1"))
        netI1bus = top.create_bus_net("netI1bus", 4, 0)
        self.assertIsNotNone(netI1bus)
        self.assertTrue(netI1bus.is_bus())
        self.assertFalse(netI1bus.is_scalar())
        self.assertFalse(netI1bus.is_const())
        #print(netI1bus.get_name())
        #self.assertEqual(len(top.get_net_list_for_bus("netI1bus")), 5)

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()