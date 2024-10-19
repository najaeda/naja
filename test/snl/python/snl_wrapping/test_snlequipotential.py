# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLEquiTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    lib = snl.SNLLibrary.create(db)
    self.primitives = snl.SNLLibrary.createPrimitives(db)
    self.top = snl.SNLDesign.create(lib)
    self.model = snl.SNLDesign.create(lib, "model")
    self.submodel = snl.SNLDesign.createPrimitive(self.primitives, "submodel")
    self.i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "I0")
    self.i1 = snl.SNLBusTerm.create(self.model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    self.o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "O")
    self.i0sub = snl.SNLScalarTerm.create(self.submodel, snl.SNLTerm.Direction.Input, "I0")
    self.i1sub = snl.SNLBusTerm.create(self.submodel, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    self.osub = snl.SNLScalarTerm.create(self.submodel, snl.SNLTerm.Direction.Output, "O")
    
  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()
    
  def testFunctions(self):
    ins2 = snl.SNLInstance.create(self.model, self.submodel, "ins2")
    ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")
    
    path0 = snl.SNLPath()
    print(path0)
    self.assertIsNotNone(path0)
    self.assertTrue(path0.empty())
    self.assertEqual(0, path0.size())
    self.assertEqual(snl.SNLPath(), path0.getHeadPath())

    instTerms = tuple(ins1.getInstTerms())
    i0Net = snl.SNLScalarNet.create(self.top, "I0")
    instTerms[0].setNet(i0Net)  
    i0Netsub = snl.SNLScalarNet.create(self.model, "I0")
    subinstTerms = tuple(ins2.getInstTerms())
    subinstTerms[0].setNet(i0Netsub)  
    self.i0.setNet(i0Netsub)  
    print(instTerms[0])  
    path1 = snl.SNLPath(path0, ins1)
    path2 = snl.SNLPath(path1, ins2)
    netcomponentoccurrence = snl.SNLNetComponentOccurrence()
    netcomponentoccurrence1 = snl.SNLNetComponentOccurrence(path1, subinstTerms[0])

    #insttermoccurrence1 = snl.SNLInstTermOccurrence(path0, instTerms[0])

    equi = snl.SNLEquipotential(netcomponentoccurrence1)
    print(equi)
    topTerms = equi.getTerms()
    for t in topTerms :
      print(t)
    insttermoccurrences = equi.getInstTermOccurrences()
    for t in insttermoccurrences :
      print(t)
    
    with self.assertRaises(RuntimeError) as context: snl.SNLEquipotential(path1)
    with self.assertRaises(RuntimeError) as context: snl.SNLEquipotential(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: snl.SNLEquipotential()
    
if __name__ == '__main__':
  unittest.main()