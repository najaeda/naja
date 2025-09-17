# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import sys
import unittest
import naja

class SNLAttributesTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    lib = naja.NLLibrary.create(db)
    self.top = naja.SNLDesign.create(lib)
    model = naja.SNLDesign.create(lib)
    naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, "I0")
    naja.SNLBusTerm.create(model, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Output, "O")
    self.ins0 = naja.SNLInstance.create(self.top, model, "ins0")
    self.ins1 = naja.SNLInstance.create(self.top, model, "ins1")

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testAttributes(self):
    pragma0 = naja.SNLAttribute("pragma0", "value0")
    self.assertEqual("pragma0", pragma0.getName())
    self.assertEqual("value0", pragma0.getValue())
    self.assertTrue(pragma0.hasValue())
    pragma1 = naja.SNLAttribute("pragma1", 10)
    self.assertEqual("pragma1", pragma1.getName())
    self.assertEqual("10", pragma1.getValue())
    self.assertTrue(pragma1.hasValue())
    pragma2 = naja.SNLAttribute("pragma2")
    self.assertEqual("pragma2", pragma2.getName())
    self.assertEqual("", pragma2.getValue())
    self.assertFalse(pragma2.hasValue())

  def testAttributesErrors(self):
    with self.assertRaises(RuntimeError) as context: naja.SNLAttribute("pragma0", "value0", "value1")
    with self.assertRaises(RuntimeError) as context: naja.SNLAttribute(3, "value0")
    with self.assertRaises(RuntimeError) as context: naja.SNLAttribute("pragma0", self.ins0)

  def test(self):
    self.assertEqual(0, sum(1 for a in self.ins0.getAttributes()))
    self.ins0.addAttribute(naja.SNLAttribute("attr0", "value0"))
    self.assertEqual(1, sum(1 for a in self.ins0.getAttributes()))
    self.ins0.addAttribute(naja.SNLAttribute("attr1", "value1"))
    self.assertEqual(2, sum(1 for a in self.ins0.getAttributes()))
    attributes = list(self.ins0.getAttributes())
    self.assertEqual("attr0", attributes[0].getName())
    self.assertEqual("value0", attributes[0].getValue())
    self.assertEqual("attr1", attributes[1].getName())
    self.assertEqual("value1", attributes[1].getValue())
    #add and test on top
    self.assertEqual(0, sum(1 for a in self.top.getAttributes()))
    self.top.addAttribute(naja.SNLAttribute("topattr", "topvalue"))
    self.assertEqual(1, sum(1 for a in self.top.getAttributes()))
    attributes = list(self.top.getAttributes())
    self.assertEqual("topattr", attributes[0].getName())
    self.assertEqual("topvalue", attributes[0].getValue())

  def testErrors(self):
    with self.assertRaises(RuntimeError) as context: self.ins0.addAttribute(3)
    with self.assertRaises(RuntimeError) as context: self.top.addAttribute(3)
  
if __name__ == '__main__':
  unittest.main()
