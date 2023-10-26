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

  def testCombi(self):
    design = snl.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    i0 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I1")
    o = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Output, "O")
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
    top = snl.SNLDesign.create(self.designs, "TOP")
    instance = snl.SNLInstance.create(top, design, "instance")
    self.assertEqual(0, sum(1 for t in snl.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i0))))
    self.assertEqual(0, sum(1 for t in snl.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i1))))
    self.assertEqual(2, sum(1 for t in snl.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))))
    self.assertEqual(0, sum(1 for t in snl.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(o))))
    inputs = [it for it in snl.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))]
    self.assertEqual(2, len(inputs))
    self.assertEqual(instance.getInstTerm(i0), inputs[0])
    self.assertEqual(instance.getInstTerm(i1), inputs[1])
    
    self.assertEqual(1, sum(1 for t in snl.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i0))))
    outputs = [t for t in snl.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i0))]
    self.assertEqual(1, len(outputs))
    self.assertEqual(instance.getInstTerm(o), outputs[0])
    self.assertEqual(1, sum(1 for t in snl.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i1))))
    outputs = [t for t in snl.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i1))]
    self.assertEqual(1, len(outputs))
    self.assertEqual(instance.getInstTerm(o), outputs[0])

  def testSeq(self):
    reg = snl.SNLDesign.createPrimitive(self.primitives, "REG")
    d = snl.SNLScalarTerm.create(reg, snl.SNLTerm.Direction.Input, "D")
    q = snl.SNLScalarTerm.create(reg, snl.SNLTerm.Direction.Output, "Q")
    c = snl.SNLScalarTerm.create(reg, snl.SNLTerm.Direction.Input, "C")
    snl.SNLDesign.addInputsToClockArcs(d, c)
    snl.SNLDesign.addClockToOutputsArcs(c, q)
    top = snl.SNLDesign.create(self.designs, "TOP")

  def testCombiWithBusses0(self):
    design = snl.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    o = snl.SNLBusTerm.create(design, snl.SNLTerm.Direction.Output, 3, 0, "O")
    d = snl.SNLBusTerm.create(design, snl.SNLTerm.Direction.Input, 3, 0, "D")
    snl.SNLDesign.addCombinatorialArcs(d, o)
    self.assertEqual(4, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(o.getBit(0))))
    self.assertEqual(4, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(d.getBit(0))))

  def testCombiWithBusses1(self):
    carry4 = snl.SNLDesign.createPrimitive(self.primitives, "CARRY4")
    o = snl.SNLBusTerm.create(carry4, snl.SNLTerm.Direction.Output, 3, 0, "O")
    co = snl.SNLBusTerm.create(carry4, snl.SNLTerm.Direction.Output, 3, 0, "CO")
    di = snl.SNLBusTerm.create(carry4, snl.SNLTerm.Direction.Input, 3, 0, "DI")
    s = snl.SNLBusTerm.create(carry4, snl.SNLTerm.Direction.Input, 3, 0, "S")
    cyinit  = snl.SNLScalarTerm.create(carry4, snl.SNLTerm.Direction.Input, "CYINIT")
    ci = snl.SNLScalarTerm.create(carry4, snl.SNLTerm.Direction.Input, "CI")
    o_bits = [b for b in o.getBits()]
    co_bits = [b for b in co.getBits()]
    di_bits = [b for b in di.getBits()] 
    s_bits = [b for b in s.getBits()] 
    #cyinit and ci are in combinatorial dependency with o and co outputs 
    snl.SNLDesign.addCombinatorialArcs([cyinit, ci], [o, co])
    snl.SNLDesign.addCombinatorialArcs(s_bits[0], [o, co])
    snl.SNLDesign.addCombinatorialArcs(s_bits[1], [o_bits[1], o_bits[2], o_bits[3]])
    snl.SNLDesign.addCombinatorialArcs(s_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    snl.SNLDesign.addCombinatorialArcs(s_bits[2], [o_bits[2], o_bits[3]])
    snl.SNLDesign.addCombinatorialArcs(s_bits[2], [co_bits[2], co_bits[3]])
    snl.SNLDesign.addCombinatorialArcs(s_bits[3], o_bits[3])
    snl.SNLDesign.addCombinatorialArcs(s_bits[3], co_bits[3])
    snl.SNLDesign.addCombinatorialArcs(di_bits[0], [o_bits[1], o_bits[2], o_bits[3]])
    snl.SNLDesign.addCombinatorialArcs(di_bits[0], co)
    snl.SNLDesign.addCombinatorialArcs(di_bits[1], [o_bits[2], o_bits[3]])
    snl.SNLDesign.addCombinatorialArcs(di_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    snl.SNLDesign.addCombinatorialArcs(di_bits[2], o_bits[3])
    snl.SNLDesign.addCombinatorialArcs(di_bits[2], [co_bits[2], co_bits[3]])
    snl.SNLDesign.addCombinatorialArcs(di_bits[3], co_bits[3])
    self.assertEqual(8, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(cyinit)))
    self.assertEqual(8, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(ci)))
    self.assertEqual(8, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(s_bits[0])))
    self.assertEqual(6, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(s_bits[1])))
    self.assertEqual(4, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(s_bits[2])))
    self.assertEqual(2, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(s_bits[3])))
    self.assertEqual(3, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(o_bits[0])))
    self.assertEqual(5, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(o_bits[1])))
    self.assertEqual(7, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(o_bits[2])))
    self.assertEqual(9, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(o_bits[3])))

  def testCombiWithBusses2(self):
    design = snl.SNLDesign.createPrimitive(self.primitives, "design")
    o = snl.SNLBusTerm.create(design, snl.SNLTerm.Direction.Output, 3, 0, "O")
    i = snl.SNLBusTerm.create(design, snl.SNLTerm.Direction.Input, 3, 0, "I")
    snl.SNLDesign.addCombinatorialArcs([i], [o])
    for o_bit in o.getBits():
      self.assertEqual(4, sum(1 for t in snl.SNLDesign.getCombinatorialInputs(o_bit)))
    for i_bit in i.getBits():
      self.assertEqual(4, sum(1 for t in snl.SNLDesign.getCombinatorialOutputs(i_bit)))


  def testSeqWithBusses0(self):
    reg = snl.SNLDesign.createPrimitive(self.primitives, "REG")
    d = snl.SNLBusTerm.create(reg, snl.SNLTerm.Direction.Input, 3, 0, "D")
    q = snl.SNLBusTerm.create(reg, snl.SNLTerm.Direction.Output, 3, 0, "Q")
    c = snl.SNLScalarTerm.create(reg, snl.SNLTerm.Direction.Input, "C")
    snl.SNLDesign.addInputsToClockArcs(d, c)
    snl.SNLDesign.addClockToOutputsArcs(c, q)
    self.assertEqual(4, sum(1 for t in snl.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(4, sum(1 for t in snl.SNLDesign.getClockRelatedOutputs(c)))

  def testSeqWithBusses1(self):
    reg = snl.SNLDesign.createPrimitive(self.primitives, "REG")
    d = snl.SNLBusTerm.create(reg, snl.SNLTerm.Direction.Input, 3, 0, "D")
    q = snl.SNLBusTerm.create(reg, snl.SNLTerm.Direction.Output, 3, 0, "Q")
    c = snl.SNLScalarTerm.create(reg, snl.SNLTerm.Direction.Input, "C")
    snl.SNLDesign.addInputsToClockArcs([d], c)
    snl.SNLDesign.addClockToOutputsArcs(c, [q])
    self.assertEqual(4, sum(1 for t in snl.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(4, sum(1 for t in snl.SNLDesign.getClockRelatedOutputs(c)))

  def testCombiErrors(self):
    design = snl.SNLDesign.createPrimitive(self.primitives, "design")
    i0 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "I1")
    o = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Output, "O")
    #wrong type
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.addCombinatorialArcs(i0, i1, o)
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.addCombinatorialArcs(design, o)
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.addCombinatorialArcs(i0, design)
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.addCombinatorialArcs([design, i0], [o, design])

  def testSeqErrors(self):
    design = snl.SNLDesign.createPrimitive(self.primitives, "design")
    d = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "D")
    q = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Output, "Q")
    c = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, "C")
    #wrong type
    snl.SNLDesign.addClockToOutputsArcs(c, q)
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.addInputsToClockArcs(d, c, q)
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.addClockToOutputsArcs(d, c, q)
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.addInputsToClockArcs(d, [c, q])
    with self.assertRaises(RuntimeError) as context: snl.SNLDesign.addClockToOutputsArcs([d, c], q)
   
if __name__ == '__main__':
  unittest.main()