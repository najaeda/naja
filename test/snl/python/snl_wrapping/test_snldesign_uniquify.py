# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLDesignUniquifyTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    lib = snl.SNLLibrary.create(db)
    self.design = snl.SNLDesign.create(lib, "DESIGN")
    i0 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLBusTerm.create(self.design, snl.SNLTerm.Direction.Input, -5, 4, "I1")
    o = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Output, "O")
    p0 = snl.SNLParameter.create_string(self.design, "P0", "Hello")
    p1 = snl.SNLParameter.create_decimal(self.design, "P1", 42)

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def testUniquifyInterface0(self):
    newDesign = self.design.uniquify()
    self.assertIsNotNone(newDesign)
    self.assertNotEqual(self.design, newDesign)
    self.assertTrue(newDesign.isAnonymous())
    self.assertEqual(self.design.getLibrary(), newDesign.getLibrary())
    self.assertEqual(self.design.getDB(), newDesign.getDB())
    termsSize = sum(1 for t in self.design.getTerms())
    newTermsSize = sum(1 for t in newDesign.getTerms())
    self.assertEqual(termsSize, newTermsSize)
    parametersSize = sum(1 for p in self.design.getParameters())
    newParametersSize = sum(1 for p in newDesign.getParameters())
    self.assertEqual(parametersSize, newParametersSize)
    
  def testUniquifyInterface1(self):
    newDesign = self.design.uniquify("uniquified")
    self.assertIsNotNone(newDesign)
    self.assertNotEqual(self.design, newDesign)
    self.assertFalse(newDesign.isAnonymous())
    self.assertEqual("uniquified", newDesign.getName())

  def testErrors(self):
    with self.assertRaises(RuntimeError) as context: self.design.uniquify("ERROR", "ERROR")

if __name__ == '__main__':
  unittest.main()