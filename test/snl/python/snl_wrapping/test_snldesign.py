# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLDesignTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    self.lib = snl.SNLLibrary.create(db)

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test0(self):
    self.assertIsNotNone(self.lib)
    self.assertFalse(any(self.lib.getDesigns()))
    self.assertEqual(0, sum(1 for d in self.lib.getDesigns()))
    design = snl.SNLDesign.create(self.lib, "DESIGN")
    self.assertIsNotNone(design)
    self.assertEqual("DESIGN", design.getName())
    self.assertEqual(self.lib, design.getLibrary())
    self.assertEqual(self.lib.getDB(), design.getDB())
    self.assertIsNotNone(self.lib.getDesign("DESIGN"))
    self.assertEqual(design, self.lib.getDesign("DESIGN"))
    self.assertTrue(any(self.lib.getDesigns()))
    self.assertEqual(1, sum(1 for d in self.lib.getDesigns()))
    designs = [d for d in self.lib.getDesigns()]
    self.assertEqual(1, len(designs))
    self.assertEqual(design, designs[0])
    self.assertFalse(any(design.getTerms()))
    self.assertEqual(0, sum(1 for d in design.getTerms()))

    i0 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I0")
    self.assertEqual(design, i0.getDesign())
    self.assertEqual(i0, design.getTerm("I0"))
    self.assertEqual(i0, design.getScalarTerm("I0"))
    self.assertIsNone(design.getBusTerm("I0"))
    self.assertTrue(any(design.getTerms()))
    self.assertEqual(1, sum(1 for d in design.getTerms()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(1, len(terms))
    self.assertEqual(i0, terms[0])

    i1 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I1")
    self.assertEqual(i1, design.getTerm("I1"))
    self.assertEqual(i1, design.getScalarTerm("I1"))
    self.assertIsNone(design.getBusTerm("I1"))
    self.assertEqual(design, i1.getDesign())
    self.assertEqual(2, sum(1 for d in design.getTerms()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(2, len(terms))
    self.assertEqual(i0, terms[0])
    self.assertEqual(i1, terms[1])

    i2 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I2")
    self.assertEqual(i2, design.getTerm("I2"))
    self.assertEqual(i2, design.getScalarTerm("I2"))
    self.assertIsNone(design.getBusTerm("I2"))
    self.assertEqual(design, i2.getDesign())
    self.assertEqual(3, sum(1 for t in design.getTerms()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(3, len(terms))
    self.assertEqual(i0, terms[0])
    self.assertEqual(i1, terms[1])
    self.assertEqual(i2, terms[2])

    i3 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I3")
    self.assertEqual(design, i3.getDesign())
    self.assertEqual(4, sum(1 for t in design.getTerms()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(4, len(terms))
    self.assertEqual(i0, terms[0])
    self.assertEqual(i1, terms[1])
    self.assertEqual(i2, terms[2])
    self.assertEqual(i3, terms[3])

    o = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Output, "O")
    self.assertEqual(design, o.getDesign())
    self.assertEqual(5, sum(1 for t in design.getTerms()))
    self.assertEqual(5, sum(1 for t in design.getBitTerms()))
    self.assertEqual(5, sum(1 for t in design.getScalarTerms()))
    self.assertEqual(0, sum(1 for t in design.getBusTerms()))
    self.assertEqual(1, sum(1 for t in o.getBits()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(5, len(terms))
    self.assertEqual(i0, terms[0])
    self.assertEqual(i1, terms[1])
    self.assertEqual(i2, terms[2])
    self.assertEqual(i3, terms[3])
    self.assertEqual(o, terms[4])
    
  def test1(self):
    self.assertIsNotNone(self.lib)
    design = snl.SNLDesign.create(self.lib, "DESIGN")
    self.assertIsNotNone(design)
    self.assertEqual("DESIGN", design.getName())
    self.assertEqual(self.lib, design.getLibrary())

    i0 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I0")
    self.assertEqual(i0, design.getTerm("I0"))
    self.assertEqual(i0, design.getScalarTerm("I0"))
    self.assertIsNone(design.getBusTerm("I0"))
    self.assertEqual(design, i0.getDesign())
    self.assertEqual(i0.getDirection(), snl.SNLTerm.Direction.Input)
    i1 = snl.SNLBusTerm.create(design, snl.SNLTerm.Direction.Input, -5, 4, "I1")
    self.assertEqual(i1, design.getTerm("I1"))
    self.assertEqual(i1, design.getBusTerm("I1"))
    self.assertIsNone(design.getScalarTerm("I1"))
    self.assertEqual(design, i1.getDesign())
    self.assertEqual(-5, i1.getMSB())
    self.assertEqual(4, i1.getLSB())
    self.assertEqual(10, i1.getSize())
    o = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Output, "O")
    self.assertEqual(design, o.getDesign())
    self.assertEqual(o.getDirection(), snl.SNLTerm.Direction.Output)
    
    self.assertEqual(3, sum(1 for t in design.getTerms()))
    self.assertEqual(1+10+1, sum(1 for b in design.getBitTerms()))
    self.assertEqual(1+1, sum(1 for t in design.getScalarTerms()))
    inputs = filter(lambda t: t.getDirection() == snl.SNLTerm.Direction.Input, design.getTerms())
    self.assertEqual(2, sum(1 for t in inputs))
    outputs = filter(lambda t: t.getDirection() == snl.SNLTerm.Direction.Output, design.getTerms())
    self.assertEqual(1, sum(1 for t in outputs))

  def testCompare(self):
    self.assertIsNotNone(self.lib)
    design0 = snl.SNLDesign.create(self.lib, "DESIGN0")
    design1 = snl.SNLDesign.create(self.lib, "DESIGN1")
    self.assertNotEqual(design0, design1)
    self.assertGreater(design1, design0)
    self.assertGreaterEqual(design1, design0)

  def testParameters(self):
    self.assertIsNotNone(self.lib)
    design = snl.SNLDesign.create(self.lib, "DESIGN")
    self.assertIsNotNone(design)

    p0 = snl.SNLParameter.create_decimal(design, "REG", 34)
    p1 = snl.SNLParameter.create_binary(design, "INIT", 16, 0x0000)
    p2 = snl.SNLParameter.create_string(design, "MODE", "DEFAULT")
    p3 = snl.SNLParameter.create_boolean(design, "INVERTED", True)
    self.assertIsNotNone(p0)
    self.assertEqual("REG", p0.getName())
    self.assertEqual(design, p0.getDesign())
    self.assertIsNotNone(p1)
    self.assertEqual("INIT", p1.getName())
    self.assertEqual(design, p1.getDesign())
    self.assertIsNotNone(p2)
    self.assertEqual("MODE", p2.getName())
    self.assertEqual(design, p2.getDesign())
    self.assertIsNotNone(p3)
    self.assertEqual("INVERTED", p3.getName())
    self.assertEqual(design, p3.getDesign())
    self.assertEqual(4, sum(1 for t in design.getParameters()))
    test_p0 = design.getParameter("REG")
    self.assertEqual(p0, test_p0)
    test_p1 = design.getParameter("INIT")
    self.assertEqual(p1, test_p1)
    p0.destroy()
    p1.destroy()
    test_p0 = design.getParameter("REG")
    self.assertIsNone(test_p0)
    test_p1 = design.getParameter("INIT")
    self.assertIsNone(test_p1)

  def testCreationError(self):
    self.assertIsNotNone(self.lib)
    d = snl.SNLDesign.create(self.lib, "DESIGN")
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.create("ERROR", "ERROR", "ERROR")
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.create(d, "DESIGN")
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.createPrimitive("ERROR", "ERROR", "ERROR")
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.createPrimitive(d, "PRIMITIVE")
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarTerm.create(d)
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarTerm.create(self.lib, snl.SNLTerm.Direction.Output, "O")
    with self.assertRaises(RuntimeError) as context: snl.SNLBusTerm.create(d)
    with self.assertRaises(RuntimeError) as context: snl.SNLBusTerm.create(self.lib, snl.SNLTerm.Direction.Output, 3, 2, "O")
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarNet.create(self.lib)
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarNet.create(self.lib, "O")
    with self.assertRaises(RuntimeError) as context: snl.SNLBusNet.create(self.lib)
    with self.assertRaises(RuntimeError) as context: snl.SNLBusNet.create(self.lib, snl.SNLTerm.Direction.Output, 3, 2, "O")

  def testParametersError(self):
    self.assertIsNotNone(self.lib)
    d = snl.SNLDesign.create(self.lib, "DESIGN")
    n = snl.SNLScalarNet.create(d, "net")
    with self.assertRaises(RuntimeError) as context: snl.SNLParameter.create_decimal(d)
    with self.assertRaises(RuntimeError) as context: snl.SNLParameter.create_binary(d)
    with self.assertRaises(RuntimeError) as context: snl.SNLParameter.create_string(d)
    with self.assertRaises(RuntimeError) as context: snl.SNLParameter.create_boolean(d)

    with self.assertRaises(RuntimeError) as context: snl.SNLParameter.create_decimal(n, "ERROR", 10)
    with self.assertRaises(RuntimeError) as context: snl.SNLParameter.create_binary(n, "ERROR", 4, 0)
    with self.assertRaises(RuntimeError) as context: snl.SNLParameter.create_string(n, "ERROR", "ERROR")
    with self.assertRaises(RuntimeError) as context: snl.SNLParameter.create_boolean(n, "ERROR", False)

  def testDestroy(self):
    design = snl.SNLDesign.create(self.lib, "DESIGN")
    design.destroy()
   
if __name__ == '__main__':
  unittest.main()