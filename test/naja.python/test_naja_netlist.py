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
from naja import netlist
from naja import snl

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

if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()
