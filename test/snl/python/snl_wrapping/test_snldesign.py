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

    i0 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I0")
    self.assertEqual(design, i0.getDesign())
    i1 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I1")
    self.assertEqual(design, i1.getDesign())
    i2 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I2")
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
    self.assertEqual(design, i0.getDesign())
    i1 = snl.SNLBusTerm.create(design, snl.SNLTerm.Direction.Input, -5, 4, "I1")
    self.assertEqual(design, i1.getDesign())
    self.assertEqual(-5, i1.getMSB())
    self.assertEqual(4, i1.getLSB())
    self.assertEqual(10, i1.getSize())
    o = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Output, "O")
    self.assertEqual(design, o.getDesign())

if __name__ == '__main__':
  unittest.main()