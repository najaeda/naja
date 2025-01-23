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

  #def test(self):
  #  self.assertIsNotNone(self.top)
  #  self.ins0.addAttribute(snl.SNLAttribute("attr0", "value0"))
  
if __name__ == '__main__':
  unittest.main()