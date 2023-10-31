# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLInstanceTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    lib = snl.SNLLibrary.create(db)
    self.top = snl.SNLDesign.create(lib)
    self.model = snl.SNLDesign.create(lib)
    i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLBusTerm.create(self.model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "O")

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test0(self):
    self.assertIsNotNone(self.top)
    self.assertIsNotNone(self.model)
    self.assertFalse(any(self.top.getInstances()))
    self.assertFalse(any(self.model.getInstances()))
    ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")
    self.assertIsNotNone(ins1)
    self.assertEqual("ins1", ins1.getName())
    self.assertEqual(self.top, ins1.getDesign())
    self.assertEqual(self.model, ins1.getModel())
    self.assertEqual(ins1, self.top.getInstance("ins1"))
    self.assertTrue(any(self.top.getInstances()))
    self.assertEqual(1, sum(1 for d in self.top.getInstances()))
    instances = [i for i in self.top.getInstances()]
    self.assertEqual(1, len(instances))
    self.assertEqual(ins1, instances[0])
    self.assertFalse(all(False for _ in ins1.getInstTerms()))

  def testErrors(self):
    with self.assertRaises(RuntimeError) as context: snl.SNLInstance.create(self.top, self.model, "ins1", "ERROR")
    with self.assertRaises(RuntimeError) as context: snl.SNLInstance.create("ERROR", self.model, "ins1")
    with self.assertRaises(RuntimeError) as context: snl.SNLInstance.create(self.top, "ERROR", "ins1")

    ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")
    with self.assertRaises(RuntimeError) as context: ins1.getInstTerm("ERROR")
    
if __name__ == '__main__':
  unittest.main()