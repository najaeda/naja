# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import sys
import unittest
import snl

class SNLAttributesTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    lib = snl.SNLLibrary.create(db)
    self.top = snl.SNLDesign.create(lib)
    model = snl.SNLDesign.create(lib)
    snl.SNLScalarTerm.create(model, snl.SNLTerm.Direction.Input, "I0")
    snl.SNLBusTerm.create(model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    snl.SNLScalarTerm.create(model, snl.SNLTerm.Direction.Output, "O")
    self.ins0 = snl.SNLInstance.create(self.top, model, "ins0")
    self.ins1 = snl.SNLInstance.create(self.top, model, "ins1")

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def testAttributes(self):
    pragma0 = snl.SNLAttribute("pragma0", "value0")
    self.assertEqual("pragma0", pragma0.getName())
    self.assertEqual("value0", pragma0.getValue())
    self.assertTrue(pragma0.hasValue())
    pragma1 = snl.SNLAttribute("pragma1", 10)
    self.assertEqual("pragma1", pragma1.getName())
    self.assertEqual("10", pragma1.getValue())
    self.assertTrue(pragma1.hasValue())
    pragma2 = snl.SNLAttribute("pragma2")
    self.assertEqual("pragma2", pragma2.getName())
    self.assertEqual("", pragma2.getValue())
    self.assertFalse(pragma2.hasValue())

  def test(self):
    self.assertEqual(0, sum(1 for a in self.ins0.getAttributes()))
    self.ins0.addAttribute(snl.SNLAttribute("attr0", "value0"))
    self.assertEqual(1, sum(1 for a in self.ins0.getAttributes()))
    self.ins0.addAttribute(snl.SNLAttribute("attr1", "value1"))
    self.assertEqual(2, sum(1 for a in self.ins0.getAttributes()))
    attributes = list(self.ins0.getAttributes())
    self.assertEqual("attr0", attributes[0].getName())
    self.assertEqual("value0", attributes[0].getValue())
    self.assertEqual("attr1", attributes[1].getName())
    self.assertEqual("value1", attributes[1].getValue())

  def testErrors(self):
    with self.assertRaises(RuntimeError) as context: self.ins0.addAttribute(3)
  
if __name__ == '__main__':
  unittest.main()