# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os

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

import unittest
import faulthandler 
from najaeda import netlist
from najaeda import snl

class NajaNetlistTest(unittest.TestCase):
  def setUp(self):
    pass

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def testLoader(self):
    desingFiles = ["../../../test/snl/formats/verilog/benchmarks/test0.v"]
    primitives = ["../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
    loader = netlist.Loader()
    loader.init()
    loader.loadLibertyPrimitives(primitives)
    loader.loadVerilog(desingFiles)
    loader.verify()
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()
  
  def testInstance(self):
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
    termsList = []
    for term in instance.getInstTerms():
      termsList.append(term)
    for term in instance.getInstTerms():
      self.assertTrue(term == termsList[index])
      index += 1
    nameList = ["ins1", "ins2"]
    instance2 = netlist.getInstanceByPath(nameList)
    self.assertEqual(instance2.path, path2)
    self.assertEqual(instance2.inst, ins2)
    print(instance2.path)
    print(instance2.inst)
    print(instance.getChildInstance(ins2.getName()).path)
    print(instance.getChildInstance(ins2.getName()).inst)
    self.assertTrue(instance2.inst == instance.getChildInstance(ins2.getName()).inst)
    self.assertTrue(instance2.path == instance.getChildInstance(ins2.getName()).path)
    self.assertTrue(instance2 == instance.getChildInstance(ins2.getName()))
  
  def testEquipotential(self):
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

    instTerms = tuple(ins1.getInstTerms())
    i0Net = snl.SNLScalarNet.create(self.top, "I0")
    instTerms[0].setNet(i0Net)  
    i0Netsub = snl.SNLScalarNet.create(self.model, "I0")
    subinstTerms = tuple(ins2.getInstTerms())
    subinstTerms[0].setNet(i0Netsub)  
    self.i0.setNet(i0Netsub)  
    print(instTerms[0])  
    path1 = snl.SNLPath(path0, ins1)
    path2 = snl.SNLPath(path1, ins2)
    instTerm = netlist.InstTerm(path1, subinstTerms[0])
    equi = netlist.Equipotential(instTerm)
    netcomponentoccurrence1 = snl.SNLNetComponentOccurrence(path1, subinstTerms[0])
    snlequi = snl.SNLEquipotential(netcomponentoccurrence1)

    snlTopTerms = []
    for t in snlequi.getTerms():
      snlTopTerms.append(t)

    for t in equi.getTopTerms():
      self.assertTrue(t.term == snlTopTerms.pop(0))
    
    snlInstTermOccurrences = []
    for t in snlequi.getInstTermOccurrences():
      snlInstTermOccurrences.append(t)
    
    for t in equi.getInstTerms():
      toCompareWith = snlInstTermOccurrences.pop(0)
      self.assertTrue(t.term == toCompareWith.getInstTerm())
      self.assertTrue(t.path == toCompareWith.getPath())

if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()
