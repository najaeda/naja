# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLPathTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    lib = naja.NLLibrary.create(db)
    self.top = naja.SNLDesign.create(lib)
    self.model = naja.SNLDesign.create(lib)
    self.submodel = naja.SNLDesign.create(lib, "submodel")

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()
    
  def testFunctions(self):
    ins2 = naja.SNLInstance.create(self.model, self.submodel, "ins2")
    ins1 = naja.SNLInstance.create(self.top, self.model, "ins1")
    
    path0 = naja.SNLPath()
    self.assertIsNotNone(path0)
    self.assertTrue(path0.empty())
    self.assertEqual(0, path0.size())
    self.assertEqual(naja.SNLPath(), path0.getHeadPath())
    self.assertEqual(naja.SNLPath(), path0.getTailPath())
    self.assertIsNone(path0.getHeadInstance())
    self.assertIsNone(path0.getTailInstance())
    
    path1 = naja.SNLPath(ins1)
    self.assertIsNotNone(path1)
    self.assertFalse(path1.empty())
    self.assertEqual(1, path1.size())
    self.assertEqual(naja.SNLPath(), path1.getHeadPath())
    self.assertEqual(naja.SNLPath(), path1.getTailPath())
    self.assertEqual(ins1, path1.getHeadInstance())
    self.assertEqual(ins1, path1.getTailInstance())

    path2 = naja.SNLPath(path1, ins2)
    
    self.assertIsNotNone(path2)
    self.assertFalse(path2.empty())
    self.assertEqual(2, path2.size())
    self.assertEqual(path1, path2.getHeadPath())
    self.assertEqual(naja.SNLPath(ins2), path2.getTailPath())
    self.assertEqual(ins1, path2.getHeadInstance())
    self.assertEqual(ins2, path2.getTailInstance())
    self.assertEqual(path2.getDesign(), ins1.getDesign())
    self.assertEqual(path2.getModel(), ins2.getModel())
    #TODO: fix commented lines
    #uniq = naja.SNLUniquifier(path2)
    #uniqPath = uniq.getPathUniqCollection()
    with self.assertRaises(RuntimeError) as context: naja.SNLPath(path1)
    with self.assertRaises(RuntimeError) as context: naja.SNLPath(path1, path2)
    with self.assertRaises(RuntimeError) as context: naja.SNLPath(1, 1, 1)
    
if __name__ == '__main__':
  unittest.main()
