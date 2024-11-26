# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import faulthandler
from naja import netlist
from naja import snl

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


class NajaNetlistTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()

    def test_loader(self):
        design_files = ["../../../test/snl/formats/verilog/benchmarks/test0.v"]
        primitives = ["../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
        loader = netlist.Loader()
        loader.init()
        loader.load_liberty_primitives(primitives)
        loader.load_verilog(design_files)
        loader.verify()
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

    def test_equipotential(self):
        universe = snl.SNLUniverse.create()
        db = snl.SNLDB.create(universe)
        lib = snl.SNLLibrary.create(db)
        self.primitives = snl.SNLLibrary.createPrimitives(db)
        self.top = snl.SNLDesign.create(lib)
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
        path2 = snl.SNLPath(path1, ins2)
        inst_term = netlist.InstTerm(path1, sub_inst_terms[0])
        equi = netlist.Equipotential(inst_term)
        net_component_occurrence1 = snl.SNLNetComponentOccurrence(path1, sub_inst_terms[0])
        snlequi = snl.SNLEquipotential(net_component_occurrence1)

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
            self.assertTrue(t.path == to_compare_with.getPath())

        instance = netlist.Instance(path1, ins1)
        print(instance.get_inst_term("I0").get_net())
        print(netlist.Net(path0, i0_net))
        self.assertTrue(instance.get_inst_term("I0").get_net() == netlist.Net(path0, i0_net))
        instance.get_inst_term("I0").disconnect()
        self.assertIsNone(instance.get_inst_term("I0").get_net().net)
        instance.get_inst_term("I0").connect(i0_net)
        self.assertTrue(instance.get_inst_term("I0").get_net() == netlist.Net(path0, i0_net))


if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
