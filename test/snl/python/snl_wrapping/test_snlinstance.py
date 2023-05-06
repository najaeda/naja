import unittest
import snl

class SNLInstanceTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    lib = snl.SNLLibrary.create(db)
    self.top = snl.SNLDesign.create(lib)
    self.model = snl.SNLDesign.create(lib)
    i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLBusTerm.create(self.model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "O")

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test0(self):
    self.assertIsNotNone(self.top)
    self.assertIsNotNone(self.model)
    ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")
    self.assertIsNotNone(ins1)
    self.assertEqual("ins1", ins1.getName())
    self.assertEqual(self.top, ins1.getDesign())
    self.assertEqual(self.model, ins1.getModel())
    self.assertEqual(ins1, self.top.getInstance("ins1"))
    
if __name__ == '__main__':
  unittest.main()