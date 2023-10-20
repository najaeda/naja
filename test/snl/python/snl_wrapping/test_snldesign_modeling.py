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
    design.addCombinatorialDependency([i0, i1], o)
    self.assertEqual(0, sum(1 for t in design.getCombinatorialInputs(i0)))
    self.assertEqual(0, sum(1 for t in design.getCombinatorialInputs(i1)))
    self.assertEqual(2, sum(1 for t in design.getCombinatorialInputs(o)))
    self.assertEqual(0, sum(1 for t in design.getCombinatorialOutputs(o)))
    inputs = [t for t in design.getCombinatorialInputs(o)]
    self.assertEqual(2, len(inputs))
    self.assertEqual(i0, inputs[0])
    self.assertEqual(i1, inputs[1])
    self.assertEqual(1, sum(1 for t in design.getCombinatorialOutputs(i0)))
    outputs = [t for t in design.getCombinatorialOutputs(i0)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])
    self.assertEqual(1, sum(1 for t in design.getCombinatorialOutputs(i1)))
    outputs = [t for t in design.getCombinatorialOutputs(i1)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])
   
if __name__ == '__main__':
  unittest.main()