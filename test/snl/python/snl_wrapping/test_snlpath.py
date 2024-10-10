# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLPathTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    lib = snl.SNLLibrary.create(db)
    self.top = snl.SNLDesign.create(lib)
    self.model = snl.SNLDesign.create(lib)
    self.submodel = snl.SNLDesign.create(lib, "submodel")
    self.i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "I0")
    self.i1 = snl.SNLBusTerm.create(self.model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    self.o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "O")
    p0 = snl.SNLParameter.create_decimal(self.model, "REG", 34)
    p1 = snl.SNLParameter.create_binary(self.model, "INIT", 16, 0x0000)
    p2 = snl.SNLParameter.create_string(self.model, "MODE", "DEFAULT")
    p3 = snl.SNLParameter.create_boolean(self.model, "INVERTED", True)

  def tearDown(self):
    del self.top
    del self.model
    del self.submodel
    del self.i0
    del self.i1
    del self.o
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()
    
  def testFunctions(self):
    self.assertEqual(4, sum(1 for p in self.model.getParameters()))
    ins2 = snl.SNLInstance.create(self.model, self.submodel, "ins2")
    ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")
    path = snl.SNLPath.createWithInstance(ins1)
    path2 = snl.SNLPath.createWithHeadAndInstance(path, ins2)
    print(path)
    print(path2)
    path.destroy()
    path2.destroy()
    with self.assertRaises(RuntimeError) as context: snl.SNLPath.createWithHeadAndInstance(ins2, path)
    with self.assertRaises(RuntimeError) as context: snl.SNLPath.createWithHeadAndInstance(path, path)
    
if __name__ == '__main__':
  unittest.main()