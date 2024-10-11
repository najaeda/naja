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
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()
    
  def testFunctions(self):
    self.assertEqual(4, sum(1 for p in self.model.getParameters()))
    ins2 = snl.SNLInstance.create(self.model, self.submodel, "ins2")
    ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")
    path0 = snl.SNLPath()
    print(path0)
    self.assertIsNotNone(path0)
    self.assertTrue(path0.empty())
    path1 = snl.SNLPath(ins1)
    print(path1)
    #self.assertFalse(path1.empty())
    #self.assertIsNotNone(path1)
    #path2 = snl.SNLPath(path1, ins2)
    #self.assertIsNotNone(path2)
    #self.assertFalse(path2.empty())
    #with self.assertRaises(RuntimeError) as context: snl.SNLPath.createWithHeadAndInstance(ins2, path)
    #with self.assertRaises(RuntimeError) as context: snl.SNLPath.createWithHeadAndInstance(path, path)
    
if __name__ == '__main__':
  unittest.main()
