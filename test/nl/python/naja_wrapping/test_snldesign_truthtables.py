# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLDesignTruthTablesTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    self.designs = naja.NLLibrary.create(db)
    self.primitives = naja.NLLibrary.createPrimitives(db)

  def tearDown(self):
    del self.designs
    del self.primitives
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testStandardGates(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "LOGIC0")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b0)
    self.assertEqual(prim.getTruthTable()[1], 0b0)
    self.assertTrue(prim.isConst0())

    prim = naja.SNLDesign.createPrimitive(self.primitives, "LOGIC1")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b1)
    self.assertTrue(prim.isConst1())

    prim = naja.SNLDesign.createPrimitive(self.primitives, "BUF")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "X")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b10)
    self.assertTrue(prim.isBuf())

    prim = naja.SNLDesign.createPrimitive(self.primitives, "INV")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "X")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b01)
    self.assertTrue(prim.isInv())

  def testPrimitiveTruthTableErrors(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "AND2")
    with self.assertRaises(RuntimeError) as context: prim.setTruthTable("ERROR")
   
if __name__ == '__main__':
  unittest.main()
