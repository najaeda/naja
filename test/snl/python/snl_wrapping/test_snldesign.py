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
    design = snl.SNLDesign.create(self.lib, "DESIGN")
    self.assertIsNotNone(design)
    self.assertEqual("DESIGN", design.getName())
    self.assertEqual(self.lib, design.getLibrary())
    self.assertIsNotNone(self.lib.getDesign("DESIGN"))
    self.assertEqual(design, self.lib.getDesign("DESIGN"))

    i0 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I0")
    self.assertEqual(design, i0.getDesign())
    self.assertEqual(i0, design.getTerm("I0"))
    self.assertEqual(i0, design.getScalarTerm("I0"))
    self.assertIsNone(design.getBusTerm("I0"))
    i1 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I1")
    self.assertEqual(i1, design.getTerm("I1"))
    self.assertEqual(i1, design.getScalarTerm("I1"))
    self.assertIsNone(design.getBusTerm("I1"))
    self.assertEqual(design, i1.getDesign())
    i2 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I2")
    self.assertEqual(i2, design.getTerm("I2"))
    self.assertEqual(i2, design.getScalarTerm("I2"))
    self.assertIsNone(design.getBusTerm("I2"))
    self.assertEqual(design, i2.getDesign())
    i3 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I3")
    self.assertEqual(design, i3.getDesign())
    o = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Output, "O")
    self.assertEqual(design, o.getDesign())

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
