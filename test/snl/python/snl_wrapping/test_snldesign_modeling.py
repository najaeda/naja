import unittest
import snl

class SNLDesignModelingTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    self.designs = snl.SNLLibrary.create(db)
    self.primitives = snl.SNLLibrary.createPrimitives(db)

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test0(self):
    top = snl.SNLDesign.create(self.designs, "TOP")
    design = snl.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    i0 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I1")
    o = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "O")
    snl.SNLDesign.addCombinatorialArcs([i0, i1], o)
    self.assertEqual(0, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(i0)))
    self.assertEqual(0, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(i1)))
    self.assertEqual(2, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(o)))
    self.assertEqual(0, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(o)))
    inputs = [t for t in snl.SNLDesign.getCombinatorialInputs(o)]
    self.assertEqual(2, len(inputs))
    self.assertEqual(i0, inputs[0])
    self.assertEqual(i1, inputs[1])
    outputs = [t for t in snl.SNLDesign.getCombinatorialOutputs(i0)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])
    outputs = [t for t in snl.SNLDesign.getCombinatorialOutputs(i1)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])

    #create instance
    instance = snl.SNLInstance.create(top, design, "instance")
    self.assertEqual(0, sum(1 for t in snl.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i0))))
    self.assertEqual(0, sum(1 for t in snl.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i1))))
    self.assertEqual(2, sum(1 for t in snl.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))))
    self.assertEqual(0, sum(1 for t in snl.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(o))))
    inputs = [it for it in instance.getCombinatorialInputs(instance.getInstTerm(o))]
    self.assertEqual(2, len(inputs))
    self.assertEqual(instance.getInstTerm(i0), inputs[0])
    self.assertEqual(instance.getInstTerm(i1), inputs[1])
    
    self.assertEqual(1, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(i0)))
    outputs = [t for t in snl.SNLDesign.getCombinatorialOutputs(i0)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])
    self.assertEqual(1, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(i1)))
    outputs = [t for t in snl.SNLDesign.getCombinatorialOutputs(i1)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])
   
if __name__ == '__main__':
  unittest.main()