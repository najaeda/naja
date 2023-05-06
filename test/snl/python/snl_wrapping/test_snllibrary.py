import unittest
import snl

class SNLLibraryTest(unittest.TestCase):
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
    #self.assertTrue(db.getLibraries.empty())

    self.assertIsNone(db.getLibrary("LIB1"))

    #Create library
    lib1 = snl.SNLLibrary.create(db, "LIB1")
    self.assertIsNotNone(lib1)
    self.assertEqual(db, lib1.getDB())
    self.assertEqual("LIB1", lib1.getName())
    self.assertEqual(lib1, db.getLibrary("LIB1"))

    #Create sublib
    lib2 = snl.SNLLibrary.create(lib1, "LIB2")
    self.assertIsNotNone(lib2)
    self.assertEqual(db, lib2.getDB())
    self.assertEqual("LIB2", lib2.getName())
    self.assertEqual(lib2, lib1.getLibrary("LIB2"))



if __name__ == '__main__':
  unittest.main()
