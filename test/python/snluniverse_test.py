import unittest
import snl

class SNLUniverseTest(unittest.TestCase):
  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test(self):
    self.assertIsNone(snl.SNLUniverse.get())
    snl.SNLUniverse.create()
    self.assertIsNotNone(snl.SNLUniverse.get())

if __name__ == '__main__':
  unittest.main()
