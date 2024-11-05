# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLOccurrenceTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    lib = snl.SNLLibrary.create(db)
    self.top = snl.SNLDesign.create(lib)
    self.model = snl.SNLDesign.create(lib)
    self.submodel = snl.SNLDesign.create(lib, "submodel")
    self.i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "I0")
    self.i1 = snl.SNLBusTerm.create(self.model, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    self.o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "O")
    
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
    
    path1 = snl.SNLPath(ins1)
    self.assertIsNotNone(path1)
    self.assertFalse(path1.empty())
    self.assertEqual(1, path1.size())
    self.assertEqual(snl.SNLPath(), path1.getHeadPath())

    path2 = snl.SNLPath(path1, ins2)
    self.assertIsNotNone(path2)
    self.assertFalse(path2.empty())
    self.assertEqual(2, path2.size())
    self.assertEqual(path1, path2.getHeadPath())

    path3 = snl.SNLPath(ins2)
    path4 = snl.SNLPath(ins1, path3)

    occurrence = snl.SNLOccurrence(path1, ins2)
    occurrence2 = snl.SNLOccurrence(ins1)
    occurrence3 = snl.SNLOccurrence()

    instTerms = tuple(ins1.getInstTerms())

    netcomponentoccurrence = snl.SNLNetComponentOccurrence()
    netcomponentoccurrence1 = snl.SNLNetComponentOccurrence(path0, instTerms[0])
    netcomponentoccurrence1 = snl.SNLNetComponentOccurrence(instTerms[0])

    insttermoccurrence = snl.SNLInstTermOccurrence()
    insttermoccurrence1 = snl.SNLInstTermOccurrence(path0, instTerms[0])
    insttermoccurrence2 = snl.SNLInstTermOccurrence(instTerms[0])
    
    uniq = snl.SNLUniquifier(insttermoccurrence1.getPath())
    uniqPath = uniq.getPathUniqCollection()

    with self.assertRaises(RuntimeError) as context: snl.SNLOccurrence(path1)
    with self.assertRaises(RuntimeError) as context: snl.SNLOccurrence(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: snl.SNLOccurrence(path1, path1)
    with self.assertRaises(RuntimeError) as context: snl.SNLNetComponentOccurrence(path1)
    with self.assertRaises(RuntimeError) as context: snl.SNLNetComponentOccurrence(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: snl.SNLNetComponentOccurrence(path1, path1)
    with self.assertRaises(RuntimeError) as context: snl.SNLInstTermOccurrence(path1)
    with self.assertRaises(RuntimeError) as context: snl.SNLInstTermOccurrence(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: snl.SNLInstTermOccurrence(path1, path1)

    with self.assertRaises(RuntimeError) as context: snl.SNLUniquifier("ERROR")
    with self.assertRaises(RuntimeError) as context: snl.SNLUniquifier()
    with self.assertRaises(RuntimeError) as context: snl.SNLUniquifier("ERROR", "ERROR")
    
if __name__ == '__main__':
  unittest.main()