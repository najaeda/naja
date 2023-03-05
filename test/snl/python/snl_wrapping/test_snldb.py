import unittest
import snl

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

  def testDestroy(self):
    u = snl.SNLUniverse.get()
    db = snl.SNLDB.create(u) 
    self.assertIsNotNone(db)
    db.destroy();


if __name__ == '__main__':
  unittest.main()