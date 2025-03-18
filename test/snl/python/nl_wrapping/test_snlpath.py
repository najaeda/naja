# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLPathTest(unittest.TestCase):
  def setUp(self):
    universe = snl.NLUniverse.create()
    db = snl.NLDB.create(universe)
    lib = snl.NLLibrary.create(db)
    self.top = snl.SNLDesign.create(lib)
    self.model = snl.SNLDesign.create(lib)
    self.submodel = snl.SNLDesign.create(lib, "submodel")

  def tearDown(self):
    if snl.NLUniverse.get():
      snl.NLUniverse.get().destroy()
    
  def testFunctions(self):
    ins2 = snl.SNLInstance.create(self.model, self.submodel, "ins2")
    ins1 = snl.SNLInstance.create(self.top, self.model, "ins1")
    
    path0 = snl.SNLPath()
    self.assertIsNotNone(path0)
    self.assertTrue(path0.empty())
    self.assertEqual(0, path0.size())
    self.assertEqual(snl.SNLPath(), path0.getHeadPath())
    self.assertEqual(snl.SNLPath(), path0.getTailPath())
    self.assertIsNone(path0.getHeadInstance())
    self.assertIsNone(path0.getTailInstance())
    
    path1 = snl.SNLPath(ins1)
    self.assertIsNotNone(path1)
    self.assertFalse(path1.empty())
    self.assertEqual(1, path1.size())
    self.assertEqual(snl.SNLPath(), path1.getHeadPath())
    self.assertEqual(snl.SNLPath(), path1.getTailPath())
    self.assertEqual(ins1, path1.getHeadInstance())
    self.assertEqual(ins1, path1.getTailInstance())

    path2 = snl.SNLPath(path1, ins2)
    
    self.assertIsNotNone(path2)
    self.assertFalse(path2.empty())
    self.assertEqual(2, path2.size())
    self.assertEqual(path1, path2.getHeadPath())
    self.assertEqual(snl.SNLPath(ins2), path2.getTailPath())
    self.assertEqual(ins1, path2.getHeadInstance())
    self.assertEqual(ins2, path2.getTailInstance())
    #TODO: fix commented lines
    #uniq = snl.SNLUniquifier(path2)
    #uniqPath = uniq.getPathUniqCollection()
    with self.assertRaises(RuntimeError) as context: snl.SNLPath(path1)
    with self.assertRaises(RuntimeError) as context: snl.SNLPath(path1, path2)
    with self.assertRaises(RuntimeError) as context: snl.SNLPath(1, 1, 1)
    
if __name__ == '__main__':
  unittest.main()