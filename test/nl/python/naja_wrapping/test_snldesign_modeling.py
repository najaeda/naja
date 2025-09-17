# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLDesignModelingTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    self.designs = naja.NLLibrary.create(db)
    self.primitives = naja.NLLibrary.createPrimitives(db)

  def tearDown(self):
    del self.designs
    del self.primitives
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testLibraries(self):
    self.assertTrue(self.designs.isStandard())
    self.assertFalse(self.designs.isPrimitives())
    self.assertTrue(self.primitives.isPrimitives())
    self.assertFalse(self.primitives.isStandard())

  def testCombi(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    i0 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([i0, i1], o)
    self.assertTrue(design.isPrimitive())
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(i0)))
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(i1)))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o)))
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(o)))
    inputs = [t for t in naja.SNLDesign.getCombinatorialInputs(o)]
    self.assertEqual(2, len(inputs))
    self.assertEqual(i0, inputs[0])
    self.assertEqual(i1, inputs[1])
    outputs = [t for t in naja.SNLDesign.getCombinatorialOutputs(i0)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])
    outputs = [t for t in naja.SNLDesign.getCombinatorialOutputs(i1)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])

    #create instance
    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, design, "instance")
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i0))))
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i1))))
    self.assertEqual(2, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))))
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(o))))
    inputs = [it for it in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))]
    self.assertEqual(2, len(inputs))
    self.assertEqual(instance.getInstTerm(i0), inputs[0])
    self.assertEqual(instance.getInstTerm(i1), inputs[1])
    
    self.assertEqual(1, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i0))))
    outputs = [t for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i0))]
    self.assertEqual(1, len(outputs))
    self.assertEqual(instance.getInstTerm(o), outputs[0])
    self.assertEqual(1, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i1))))
    outputs = [t for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i1))]
    self.assertEqual(1, len(outputs))
    self.assertEqual(instance.getInstTerm(o), outputs[0])

  def testSeq(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d0 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "D0")
    q0 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Output, "Q0")
    d1 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "D1")
    q1 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Output, "Q1")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs(d0, c)
    naja.SNLDesign.addClockToOutputsArcs(c, q0)
    naja.SNLDesign.addInputsToClockArcs([d1], c)
    naja.SNLDesign.addClockToOutputsArcs(c, [q1])
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))

  def testCombiWithBusses0(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    o = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Output, 3, 0, "O")
    d = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Input, 3, 0, "D")
    naja.SNLDesign.addCombinatorialArcs(d, o)
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o.getBusTermBit(0))))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(d.getBusTermBit(0))))

  def testCombiWithBusses1(self):
    carry4 = naja.SNLDesign.createPrimitive(self.primitives, "CARRY4")
    o = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Output, 3, 0, "O")
    co = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Output, 3, 0, "CO")
    di = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Input, 3, 0, "DI")
    s = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Input, 3, 0, "S")
    cyinit  = naja.SNLScalarTerm.create(carry4, naja.SNLTerm.Direction.Input, "CYINIT")
    ci = naja.SNLScalarTerm.create(carry4, naja.SNLTerm.Direction.Input, "CI")
    o_bits = [b for b in o.getBits()]
    co_bits = [b for b in co.getBits()]
    di_bits = [b for b in di.getBits()] 
    s_bits = [b for b in s.getBits()] 
    #cyinit and ci are in combinatorial dependency with o and co outputs 
    naja.SNLDesign.addCombinatorialArcs([cyinit, ci], [o, co])
    naja.SNLDesign.addCombinatorialArcs(s_bits[0], [o, co])
    naja.SNLDesign.addCombinatorialArcs(s_bits[1], [o_bits[1], o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[2], [o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[2], [co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[3], o_bits[3])
    naja.SNLDesign.addCombinatorialArcs(s_bits[3], co_bits[3])
    naja.SNLDesign.addCombinatorialArcs(di_bits[0], [o_bits[1], o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[0], co)
    naja.SNLDesign.addCombinatorialArcs(di_bits[1], [o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[2], o_bits[3])
    naja.SNLDesign.addCombinatorialArcs(di_bits[2], [co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[3], co_bits[3])
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(cyinit)))
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(ci)))
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[0])))
    self.assertEqual(6, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[1])))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[2])))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[3])))
    self.assertEqual(3, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[0])))
    self.assertEqual(5, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[1])))
    self.assertEqual(7, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[2])))
    self.assertEqual(9, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[3])))

  def testCombiWithBusses2(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    o = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Output, 3, 0, "O")
    i = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Input, 3, 0, "I")
    naja.SNLDesign.addCombinatorialArcs([i], [o])
    for o_bit in o.getBits():
      self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bit)))
    for i_bit in i.getBits():
      self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(i_bit)))

  def testSeqWithBusses0(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Input, 3, 0, "D")
    q = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Output, 3, 0, "Q")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs(d, c)
    naja.SNLDesign.addClockToOutputsArcs(c, q)
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))

  def testSeqWithBusses1(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Input, 3, 0, "D")
    q = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Output, 3, 0, "Q")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs([d], c)
    naja.SNLDesign.addClockToOutputsArcs(c, [q])
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))

  def testCreationErrors(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "design")
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.createPrimitive(self.designs, "design")

  def testCombiErrors(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    i0 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "O")
    #wrong type
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(i0, i1, o)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(design, o)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(i0, design)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs([design, i0], [o, design])

  def testSeqErrors(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    d = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "D")
    q = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "Q")
    c = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "C")
    #wrong type
    naja.SNLDesign.addClockToOutputsArcs(c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(d, c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(d, [c, q])
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(design, c)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs([design], c)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(d, c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs([d, c], q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(c, design)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(c, [design])
    with self.assertRaises(RuntimeError) as context: design.getCombinatorialInputs(design)
    with self.assertRaises(RuntimeError) as context: design.getCombinatorialOutputs(design)

    #create instance
    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, design, "instance")
    with self.assertRaises(RuntimeError) as context: instance.getCombinatorialInputs(d)
    with self.assertRaises(RuntimeError) as context: instance.getCombinatorialOutputs(q)
   
if __name__ == '__main__':
  unittest.main()
