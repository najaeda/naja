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
    #self.assertEquals(0, lib1.getID())
    self.assertEqual("LIB1", lib1.getName())
    self.assertEqual(lib1, db.getLibrary("LIB1"))


if __name__ == '__main__':
  unittest.main()
