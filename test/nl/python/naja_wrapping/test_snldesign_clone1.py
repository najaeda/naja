# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLDesignCloneTest1(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    lib = naja.NLLibrary.create(db)
    self.design = naja.SNLDesign.create(lib, "DESIGN")
    i0 = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLBusTerm.create(self.design, naja.SNLTerm.Direction.Input, -5, 4, "I1")
    o = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Output, "O")
    p0 = naja.SNLParameter.create_string(self.design, "P0", "Hello")
    p1 = naja.SNLParameter.create_decimal(self.design, "P1", 42)

  def tearDown(self):
    del self.design
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testCloneInterface0(self):
    newDesign = self.design.clone()
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
    
  def testCloneInterface1(self):
    newDesign = self.design.clone("cloned")
    self.assertIsNotNone(newDesign)
    self.assertNotEqual(self.design, newDesign)
    self.assertFalse(newDesign.isAnonymous())
    self.assertEqual("cloned", newDesign.getName())

  def testErrors(self):
    with self.assertRaises(RuntimeError) as context: self.design.clone("ERROR", "ERROR")
    newDesign = self.design.clone("cloned")
    self.assertIsNotNone(newDesign)
    with self.assertRaises(RuntimeError) as context: self.design.clone("cloned")

if __name__ == '__main__':
  unittest.main()
