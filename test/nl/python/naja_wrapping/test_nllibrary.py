# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class NLLibraryTest(unittest.TestCase):
  def setUp(self):
    naja.NLUniverse.create()

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def test(self):
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(db)
    self.assertTrue(all(False for _ in db.getLibraries()))

    self.assertIsNone(db.getLibrary("LIB1"))

    #Create library
    lib1 = naja.NLLibrary.create(db, "LIB1")
    self.assertIsNotNone(lib1)
    self.assertEqual(0, lib1.getID())
    self.assertEqual(db, lib1.getDB())
    self.assertEqual("LIB1", lib1.getName())
    self.assertEqual(lib1, db.getLibrary("LIB1"))
    self.assertEqual(lib1, db.getLibrary(0))
    self.assertFalse(any(lib1.getSNLDesigns()))
    self.assertTrue(lib1.isStandard())
    self.assertFalse(lib1.isPrimitives())
    self.assertListEqual([lib1], list(db.getLibraries()))
    self.assertListEqual([], list(db.getPrimitiveLibraries()))

    lib1.setName('RENAMEDLIB1')
    self.assertIsNone(db.getLibrary("LIB1"))
    self.assertEqual(lib1, db.getLibrary("RENAMEDLIB1"))

    #Create sublib
    lib2 = naja.NLLibrary.create(lib1, "LIB2")
    self.assertIsNotNone(lib2)
    self.assertEqual(db, lib2.getDB())
    self.assertEqual("LIB2", lib2.getName())
    self.assertEqual(lib2, lib1.getLibrary("LIB2"))
    self.assertTrue(lib2.isStandard())
    self.assertFalse(lib2.isPrimitives())
    self.assertListEqual([lib1, lib2], list(db.getGlobalLibraries()))

    #create anonymous sublib
    anonajaib = naja.NLLibrary.create(lib1)
    self.assertIsNotNone(anonajaib)

  def testErrors(self):
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(db)

    #Create library errors
    with self.assertRaises(RuntimeError) as context: lib1 = naja.NLLibrary.create(db, "LIB1", "ERROR")
    with self.assertRaises(RuntimeError) as context: lib1 = naja.NLLibrary.create(u, "LIB1")

    lib = naja.NLLibrary.create(db, "LIB")
    with self.assertRaises(RuntimeError) as context: lib1 = naja.NLLibrary.create(db, "LIB")
    with self.assertRaises(RuntimeError) as context: db.getLibrary(db)
    del lib

if __name__ == '__main__':
  unittest.main()
