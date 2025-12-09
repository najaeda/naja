# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLEquiTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    lib = naja.NLLibrary.create(db)
    self.primitives = naja.NLLibrary.createPrimitives(db)
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
    
  def testFunctions(self):
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
    topTerms = [t for t in equi1.getTerms()]
    self.assertListEqual([self.top_out], topTerms)
    insttermoccurrences = [i for i in equi1.getInstTermOccurrences()]
    self.assertEqual(1, len(insttermoccurrences))
    self.assertListEqual([netcomponentoccurrence1], insttermoccurrences)
    
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
