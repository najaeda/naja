# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLDesignTruthTablesTest(unittest.TestCase):
  def setUp(self):
    universe = snl.NLUniverse.create()
    db = snl.NLDB.create(universe)
    self.designs = snl.NLLibrary.create(db)
    self.primitives = snl.NLLibrary.createPrimitives(db)

  def tearDown(self):
    del self.designs
    del self.primitives
    if snl.NLUniverse.get():
      snl.NLUniverse.get().destroy()

  def testStandardGates(self):
    prim = snl.SNLDesign.createPrimitive(self.primitives, "LOGIC0")
    snl.SNLScalarTerm.create(prim, snl.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b0)
    self.assertEqual(prim.getTruthTable()[1], 0b0)
    self.assertTrue(prim.isConst0())

    prim = snl.SNLDesign.createPrimitive(self.primitives, "LOGIC1")
    snl.SNLScalarTerm.create(prim, snl.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b1)
    self.assertTrue(prim.isConst1())

    prim = snl.SNLDesign.createPrimitive(self.primitives, "BUF")
    snl.SNLScalarTerm.create(prim, snl.SNLTerm.Direction.Input, "X")
    snl.SNLScalarTerm.create(prim, snl.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b10)
    self.assertTrue(prim.isBuf())

    prim = snl.SNLDesign.createPrimitive(self.primitives, "INV")
    snl.SNLScalarTerm.create(prim, snl.SNLTerm.Direction.Input, "X")
    snl.SNLScalarTerm.create(prim, snl.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b01)
    self.assertTrue(prim.isInv())

  def testPrimitiveTruthTableErrors(self):
    prim = snl.SNLDesign.createPrimitive(self.primitives, "AND2")
    with self.assertRaises(RuntimeError) as context: prim.setTruthTable("ERROR")
   
if __name__ == '__main__':
  unittest.main()