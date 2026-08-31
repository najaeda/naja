# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest
import naja

class SNLEquiTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    self.db = naja.NLDB.create(universe)
    lib = naja.NLLibrary.create(self.db)
    self.primitives = naja.NLLibrary.createPrimitives(self.db)
    self.top = naja.SNLDesign.create(lib)
    self.top_out = naja.SNLScalarTerm.create(self.top, naja.SNLTerm.Direction.Output, "OUT")
    self.model = naja.SNLDesign.create(lib, "model")
    self.submodel = naja.SNLDesign.createPrimitive(self.primitives, "submodel")
    self.i0 = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Input, "I0")
    self.i1 = naja.SNLBusTerm.create(self.model, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    self.o = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Output, "O")
    self.i0sub = naja.SNLScalarTerm.create(self.submodel, naja.SNLTerm.Direction.Input, "I0")
    self.i1sub = naja.SNLBusTerm.create(self.submodel, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    self.osub = naja.SNLScalarTerm.create(self.submodel, naja.SNLTerm.Direction.Output, "O")
    
  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()
    
  def test(self):
    ins2 = naja.SNLInstance.create(self.model, self.submodel, "ins2")
    ins1 = naja.SNLInstance.create(self.top, self.model, "ins1")
    
    path0 = naja.SNLPath()
    print(path0)
    self.assertIsNotNone(path0)
    self.assertTrue(path0.empty())
    self.assertEqual(0, path0.size())
    self.assertEqual(naja.SNLPath(), path0.getHeadPath())

    instTerms = tuple(ins1.getInstTerms())
    i0Net = naja.SNLScalarNet.create(self.top, "I0")
    self.top_out.setNet(i0Net)
    instTerms[0].setNet(i0Net)  
    i0Netsub = naja.SNLScalarNet.create(self.model, "I0")
    subinstTerms = tuple(ins2.getInstTerms())
    subinstTerms[0].setNet(i0Netsub)  
    self.i0.setNet(i0Netsub)  
    #print(instTerms[0])  
    path1 = naja.SNLPath(path0, ins1)
    path2 = naja.SNLPath(path1, ins2)
    netcomponentoccurrence1 = naja.SNLOccurrence(path1, subinstTerms[0])

    #insttermoccurrence1 = naja.SNLInstTermOccurrence(path0, instTerms[0])

    equi0 = naja.SNLEquipotential(netcomponentoccurrence1)
    topTerms = [t for t in equi0.getTerms()]
    self.assertListEqual([self.top_out], topTerms)
    insttermoccurrences = [i for i in equi0.getInstTermOccurrences()]
    self.assertEqual(1, len(insttermoccurrences))
    self.assertListEqual([netcomponentoccurrence1], insttermoccurrences)

    equi1 = naja.SNLEquipotential(self.top_out)
    self.assertEqual(equi0, equi1)
    with self.assertRaises(TypeError):
      hash(equi0)
    topTerms = [t for t in equi1.getTerms()]
    self.assertListEqual([self.top_out], topTerms)
    insttermoccurrences = [i for i in equi1.getInstTermOccurrences()]
    self.assertEqual(1, len(insttermoccurrences))
    self.assertListEqual([netcomponentoccurrence1], insttermoccurrences)

  def testConstant0TopTerm(self):
    i0Net = naja.SNLScalarNet.create(self.top, "I0")
    i0Net.setType(naja.SNLNet.Type.Assign0)
    self.top_out.setNet(i0Net)
    equi = naja.SNLEquipotential(self.top_out)
    self.assertTrue(equi.isConst0())

  def testConstant1TopTerm(self):
    i0Net = naja.SNLScalarNet.create(self.top, "I0")
    i0Net.setType(naja.SNLNet.Type.Assign1)
    self.top_out.setNet(i0Net)
    equi = naja.SNLEquipotential(self.top_out)
    self.assertTrue(equi.isConst1())

  def testConstantXAndZTopTerm(self):
    net = naja.SNLScalarNet.create(self.top, "four_state")
    self.top_out.setNet(net)
    net.setType(naja.SNLNet.Type.AssignX)
    equi_x = naja.SNLEquipotential(self.top_out)
    self.assertTrue(equi_x.isConstX())
    self.assertFalse(equi_x.isConstZ())
    net.setType(naja.SNLNet.Type.AssignZ)
    equi_z = naja.SNLEquipotential(self.top_out)
    self.assertTrue(equi_z.isConstZ())
    self.assertFalse(equi_z.isConstX())

  def testTraverseAssigns(self):
    with tempfile.NamedTemporaryFile("w", suffix=".v", delete=False) as source:
      source.write(
        "module assign_top(input i, output o); "
        "wire n; assign n = i; assign o = n; endmodule\n")
      source_path = source.name
    try:
      top = self.db.loadVerilog([source_path])
      input_term = top.getScalarTerm("i")
      output_term = top.getScalarTerm("o")

      standard = naja.SNLEquipotential(input_term)
      self.assertListEqual([input_term], list(standard.getTerms()))
      assign_occurrences = list(standard.getInstTermOccurrences())
      self.assertEqual(1, len(assign_occurrences))
      self.assertEqual(
        standard,
        naja.SNLEquipotential(
          input_term, mode=naja.SNLEquipotential.Mode.Standard))

      traversed = naja.SNLEquipotential(
        input_term, mode=naja.SNLEquipotential.Mode.TraverseAssigns)
      self.assertCountEqual(
        [input_term, output_term], list(traversed.getTerms()))
      self.assertListEqual([], list(traversed.getInstTermOccurrences()))
      self.assertEqual(
        traversed,
        naja.SNLEquipotential(
          output_term, naja.SNLEquipotential.Mode.TraverseAssigns))
      self.assertEqual(
        traversed,
        naja.SNLEquipotential(
          assign_occurrences[0],
          mode=naja.SNLEquipotential.Mode.TraverseAssigns))
      with self.assertRaisesRegex(
          RuntimeError, "invalid SNLEquipotential.Mode value"):
        naja.SNLEquipotential(input_term, mode=99)
      with self.assertRaisesRegex(
          RuntimeError,
          "SNLEquipotential mode must be an SNLEquipotential.Mode"):
        naja.SNLEquipotential(input_term, mode="TraverseAssigns")
    finally:
      os.remove(source_path)

  def testErrors(self):
    ins = naja.SNLInstance.create(self.model, self.submodel, "ins")
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(0)
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(ins)
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(naja.SNLOccurrence(ins))
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(naja.SNLPath())
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(naja.SNLOccurrence())
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential()
    
if __name__ == '__main__':
  unittest.main()
