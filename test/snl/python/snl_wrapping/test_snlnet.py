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
    i1 = snl.SNLBusTerm.create(self.design, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    o = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Output, "O")

    self.assertIsNone(i0.getNet())
    i0Net = snl.SNLScalarNet.create(self.design, "I0")
    self.assertIsNotNone(i0Net)
    self.assertEqual(self.design, i0Net.getDesign())
    self.assertEqual("I0", i0Net.getName())
    i0.setNet(i0Net)
    self.assertEqual(i0.getNet(), i0Net)
    self.assertEqual(i0Net, i0.getNet())
    self.assertEqual(i0Net, self.design.getNet("I0"))
    self.assertEqual(i0Net, self.design.getScalarNet("I0"))
    self.assertIsNone(self.design.getBusNet("I0"))

    i1Net = snl.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertIsNotNone(i1Net)
    self.assertEqual("I1", i1Net.getName())
    self.assertEqual(4, i1.getMSB())
    self.assertEqual(0, i1.getLSB())
    self.assertEqual(5, i1.getSize())
    self.assertEqual(4, i1Net.getMSB())
    self.assertEqual(0, i1Net.getLSB())
    self.assertEqual(5, i1Net.getSize())
    self.assertEqual(i1Net, self.design.getNet("I1"))
    self.assertEqual(i1Net, self.design.getBusNet("I1"))
    self.assertIsNone(self.design.getScalarNet("I1"))
    i1.setNet(i1Net)
    
if __name__ == '__main__':
  unittest.main()