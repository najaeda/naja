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

  def testGateFamilyPredicates(self):
    def create_gate(name, mask):
      prim = naja.SNLDesign.createPrimitive(self.primitives, name)
      naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "A")
      naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "B")
      naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
      prim.setTruthTable(mask)
      return prim

    and2 = create_gate("AND2", 0x8)
    self.assertTrue(and2.isAnd())
    self.assertFalse(and2.isXor())

    nand2 = create_gate("NAND2", 0x7)
    self.assertTrue(nand2.isNand())
    self.assertFalse(nand2.isAnd())

    or2 = create_gate("OR2", 0xE)
    self.assertTrue(or2.isOr())
    self.assertFalse(or2.isNor())

    nor2 = create_gate("NOR2", 0x1)
    self.assertTrue(nor2.isNor())
    self.assertFalse(nor2.isOr())

    xor2 = create_gate("XOR2", 0x6)
    self.assertTrue(xor2.isXor())
    self.assertFalse(xor2.isXnor())

    xnor2 = create_gate("XNOR2", 0x9)
    self.assertTrue(xnor2.isXnor())
    self.assertFalse(xnor2.isXor())

  def testPrimitiveTruthTableErrors(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "AND2")
    with self.assertRaises(RuntimeError) as context: prim.setTruthTable("ERROR")
   
if __name__ == '__main__':
  unittest.main()
