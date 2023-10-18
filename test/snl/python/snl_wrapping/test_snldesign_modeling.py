import unittest
import snl

class SNLDesignModelingTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    self.lib = snl.SNLLibrary.createPrimitives(db)

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test0(self):
    design = snl.SNLDesign.createPrimitive(self.lib, "DESIGN")
    i0 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I1")
    o = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "O")
    design.addCombinatorialDependency(i1, o)
    design.addCombinatorialDependency(i0, o)
   
if __name__ == '__main__':
  unittest.main()