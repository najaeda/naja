# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLDesignTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    self.lib = naja.NLLibrary.create(db)

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()
      del self.lib

  def test0(self):
    self.assertIsNotNone(self.lib)
    self.assertFalse(any(self.lib.getDesigns()))
    self.assertEqual(0, sum(1 for d in self.lib.getDesigns()))
    design = naja.SNLDesign.create(self.lib, "DESIGN")
    self.assertIsNotNone(design)
    self.assertEqual("DESIGN", design.getName())
    self.assertEqual(0, design.getID())
    self.assertFalse(design.isPrimitive())
    self.assertEqual(self.lib, design.getLibrary())
    self.assertEqual(self.lib.getDB(), design.getDB())
    self.assertIsNotNone(self.lib.getDesign("DESIGN"))
    self.assertEqual(design, self.lib.getDesign("DESIGN"))
    self.assertEqual(design, self.lib.getDesign(0))
    self.assertTrue(any(self.lib.getDesigns()))
    self.assertEqual(1, sum(1 for d in self.lib.getDesigns()))
    designs = [d for d in self.lib.getDesigns()]
    self.assertEqual(1, len(designs))
    self.assertEqual(design, designs[0])
    self.assertFalse(any(design.getTerms()))
    self.assertEqual(0, sum(1 for d in design.getTerms()))

    i0 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")
    self.assertEqual(design, i0.getDesign())
    self.assertEqual(i0, design.getTerm("I0"))
    self.assertEqual(i0, design.getScalarTerm("I0"))
    self.assertIsNone(design.getBusTerm("I0"))
    self.assertTrue(any(design.getTerms()))
    self.assertEqual(1, sum(1 for d in design.getTerms()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(1, len(terms))
    self.assertEqual(i0, terms[0])
    with self.assertRaises(RuntimeError) as context: naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")

    i1 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I1")
    self.assertEqual(i1, design.getTerm("I1"))
    self.assertEqual(i1, design.getScalarTerm("I1"))
    self.assertIsNone(design.getBusTerm("I1"))
    self.assertEqual(design, i1.getDesign())
    self.assertEqual(2, sum(1 for d in design.getTerms()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(2, len(terms))
    self.assertEqual(i0, terms[0])
    self.assertEqual(i1, terms[1])
    self.assertEqual(0, terms[0].getID())
    self.assertEqual(0, terms[0].getBit())

    i2 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I2")
    self.assertEqual(i2, design.getTerm("I2"))
    self.assertEqual(i2, design.getScalarTerm("I2"))
    self.assertEqual(2, design.getScalarTerm("I2").getID())
    self.assertIsNone(design.getBusTerm("I2"))
    self.assertEqual(design, i2.getDesign())
    self.assertEqual(3, sum(1 for t in design.getTerms()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(3, len(terms))
    self.assertEqual(i0, terms[0])
    self.assertEqual(i1, terms[1])
    self.assertEqual(i2, terms[2])

    i3 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I3")
    self.assertEqual(design, i3.getDesign())
    self.assertEqual(4, sum(1 for t in design.getTerms()))
    terms = [t for t in design.getTerms()]
    self.assertEqual(4, len(terms))
    self.assertEqual(i0, terms[0])
    self.assertEqual(i1, terms[1])
    self.assertEqual(i2, terms[2])
    self.assertEqual(i3, terms[3])

    o = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "O")
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

    self.assertIsNotNone(naja.NLUniverse.get())
    self.assertIsNone(naja.NLUniverse.get().getTopDB())
    self.assertIsNone(naja.NLUniverse.get().getTopDesign())
    naja.NLUniverse.get().setTopDesign(design)
    self.assertTrue(design.isTopDesign())
    self.assertEqual(design, naja.NLUniverse.get().getTopDesign())
    self.assertEqual(design.getDB(), naja.NLUniverse.get().getTopDB())
    with self.assertRaises(RuntimeError) as context: naja.NLUniverse.get().setTopDesign(self.lib)
    self.assertEqual(len(design.getNLID()), 6)

  def test1(self):
    self.assertIsNotNone(self.lib)
    design = naja.SNLDesign.create(self.lib, "DESIGN")
    with self.assertRaises(RuntimeError) as context: design.getInstanceByIDList()
    with self.assertRaises(RuntimeError) as context: design.getInstanceByIDList("ERROR")
    self.assertIsNotNone(design)
    self.assertEqual("DESIGN", design.getName())
    self.assertEqual(self.lib, design.getLibrary())

    i0 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")
    self.assertEqual(i0, design.getTerm("I0"))
    self.assertEqual(i0, design.getScalarTerm("I0"))
    self.assertIsNone(design.getBusTerm("I0"))
    self.assertEqual(design, i0.getDesign())
    self.assertEqual(i0.getDirection(), naja.SNLTerm.Direction.Input)
    i1 = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Input, -5, 4, "I1")
    self.assertEqual(i1, design.getTerm("I1"))
    self.assertEqual(i1, design.getBusTerm("I1"))
    self.assertEqual(1, design.getBusTerm("I1").getID())
    self.assertIsNone(design.getScalarTerm("I1"))
    self.assertEqual(design, i1.getDesign())
    self.assertEqual(-5, i1.getMSB())
    self.assertEqual(4, i1.getLSB())
    self.assertEqual(10, i1.getWidth())
    o = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "O")
    self.assertEqual(design, o.getDesign())
    self.assertEqual(o.getDirection(), naja.SNLTerm.Direction.Output)
    
    self.assertEqual(3, sum(1 for t in design.getTerms()))
    self.assertEqual(1+10+1, sum(1 for b in design.getBitTerms()))
    self.assertEqual(1+1, sum(1 for t in design.getScalarTerms()))
    inputs = filter(lambda t: t.getDirection() == naja.SNLTerm.Direction.Input, design.getTerms())
    self.assertEqual(2, sum(1 for t in inputs))
    outputs = filter(lambda t: t.getDirection() == naja.SNLTerm.Direction.Output, design.getTerms())
    self.assertEqual(1, sum(1 for t in outputs))
    

  def testCompare(self):
    self.assertIsNotNone(self.lib)
    design0 = naja.SNLDesign.create(self.lib, "DESIGN0")
    design1 = naja.SNLDesign.create(self.lib, "DESIGN1")
    self.assertNotEqual(design0, design1)
    self.assertGreater(design1, design0)
    self.assertGreaterEqual(design1, design0)

  def testParameters(self):
    self.assertIsNotNone(self.lib)
    design = naja.SNLDesign.create(self.lib, "DESIGN")
    self.assertIsNotNone(design)

    p0 = naja.SNLParameter.create_decimal(design, "REG", 34)
    p1 = naja.SNLParameter.create_binary(design, "INIT", 16, 0x0000)
    p2 = naja.SNLParameter.create_string(design, "MODE", "DEFAULT")
    p3 = naja.SNLParameter.create_boolean(design, "INVERTED", True)
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
    d = naja.SNLDesign.create(self.lib, "DESIGN")
    with self.assertRaises(RuntimeError) as context: self.lib.getDesign(self.lib)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.create("ERROR", "ERROR", "ERROR")
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.create(d, "DESIGN")
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.create(self.lib, "DESIGN")
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.createPrimitive("ERROR", "ERROR", "ERROR")
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.createPrimitive(d, "PRIMITIVE")
    with self.assertRaises(RuntimeError) as context: naja.SNLScalarTerm.create(d)
    with self.assertRaises(RuntimeError) as context: naja.SNLScalarTerm.create(self.lib, naja.SNLTerm.Direction.Output, "O")
    with self.assertRaises(RuntimeError) as context: naja.SNLBusTerm.create(d)
    with self.assertRaises(RuntimeError) as context: naja.SNLBusTerm.create(self.lib, naja.SNLTerm.Direction.Output, 3, 2, "O")
    with self.assertRaises(RuntimeError) as context: naja.SNLScalarNet.create(self.lib)
    with self.assertRaises(RuntimeError) as context: naja.SNLScalarNet.create(self.lib, "O")
    with self.assertRaises(RuntimeError) as context: naja.SNLBusNet.create(self.lib)
    with self.assertRaises(RuntimeError) as context: naja.SNLBusNet.create(self.lib, naja.SNLTerm.Direction.Output, 3, 2, "O")

  def testParametersError(self):
    self.assertIsNotNone(self.lib)
    d = naja.SNLDesign.create(self.lib, "DESIGN")
    n = naja.SNLScalarNet.create(d, "net")
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_decimal(d)
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_binary(d)
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_string(d)
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_boolean(d)

    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_decimal(n, "ERROR", 10)
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_binary(n, "ERROR", 4, 0)
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_string(n, "ERROR", "ERROR")
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_boolean(n, "ERROR", False)

  def testParameterClashErrors(self):
    self.assertIsNotNone(self.lib)
    design = naja.SNLDesign.create(self.lib, "DESIGN")
    self.assertIsNotNone(design)
    p0 = naja.SNLParameter.create_decimal(design, "REG", 34)
    p1 = naja.SNLParameter.create_binary(design, "INIT", 16, 0x0000)
    p2 = naja.SNLParameter.create_string(design, "MODE", "DEFAULT")
    p3 = naja.SNLParameter.create_boolean(design, "INVERTED", True)
    self.assertIsNotNone(p0)
    self.assertIsNotNone(p1)
    self.assertIsNotNone(p2)
    self.assertIsNotNone(p3)
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_decimal(design, "REG", 34)
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_binary(design, "INIT", 16, 0x0000)
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_string(design, "MODE", "DEFAULT")
    with self.assertRaises(RuntimeError) as context: naja.SNLParameter.create_boolean(design, "INVERTED", True)

  def testSetTopErrors(self):
    with self.assertRaises(RuntimeError) as context: naja.NLUniverse.get().setTopDesign(self.lib)

  def testDumpVerilogError(self):
    self.assertIsNotNone(self.lib)
    design = naja.SNLDesign.create(self.lib, "DESIGN")
    with self.assertRaises(RuntimeError) as context: design.dumpVerilog("ERROR")

  def testDestroy(self):
    design = naja.SNLDesign.create(self.lib, "DESIGN")
    design.destroy()
   
if __name__ == '__main__':
  unittest.main()
