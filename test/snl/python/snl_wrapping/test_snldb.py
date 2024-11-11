# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl
import faulthandler 

class SNLDBTest(unittest.TestCase):
  def setUp(self):
    snl.SNLUniverse.create()

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test(self):
    u = snl.SNLUniverse.get()
    self.assertIsNotNone(u)
    db = snl.SNLDB.create(u) 
    self.assertIsNotNone(db)
    db.dumpSNL("./test_snl")
    del db    

  def testLoad(self):
    u = snl.SNLUniverse.get()
    self.assertIsNotNone(u)
    db1 = snl.SNLDB.loadSNL("./test_snl")
    self.assertIsNotNone(db1)
    del db1

  def testDestroy(self):
    u = snl.SNLUniverse.get()
    self.assertIsNotNone(u)
    db = snl.SNLDB.create(u) 
    self.assertIsNotNone(db)
    db.destroy()

  def testCreationError(self):
    u = snl.SNLUniverse.get()
    #db = snl.SNLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.create()
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.create("ERROR")
    u.destroy()
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.create(u)
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL()
    #with self.assertRaises(RuntimeError) as context: db.dumpSNL()

if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()