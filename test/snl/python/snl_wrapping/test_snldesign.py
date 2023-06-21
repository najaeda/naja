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

    self.assertEqual(3, sum(1 for t in design.getTerms()))
    self.assertEqual(1+10+1, sum(1 for b in design.getBitTerms()))
    self.assertEqual(1+1, sum(1 for t in design.getScalarTerms()))

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

  def testCreationError(self):
    self.assertIsNotNone(self.lib)
    d = snl.SNLDesign.create(self.lib, "DESIGN")
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.create("ERROR", "DESIGN")
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.create(d, "DESIGN")

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
   
if __name__ == '__main__':
  unittest.main()