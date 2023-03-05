import unittest
import snl

class SNLNetTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    lib = snl.SNLLibrary.create(db)
    self.design = snl.SNLDesign.create(lib)

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test0(self):
    self.assertIsNotNone(self.design)

    i0 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Input, "I1")
    o = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Output, "O")

    self.assertIsNone(i0.getNet())
    i0Net = snl.SNLScalarNet.create(self.design, "I0")
    self.assertIsNotNone(i0Net)
    self.assertEqual(self.design, i0Net.getDesign())
    i0.setNet(i0Net)
    self.assertEqual(i0.getNet(), i0.getNet())
    self.assertEqual(i0Net, i0.getNet())

if __name__ == '__main__':
  unittest.main()