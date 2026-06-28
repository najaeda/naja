# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLUniverseTest(unittest.TestCase):
  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testVersion(self):
    self.assertIsNotNone(naja.getVersion())
    self.assertIsNotNone(naja.getGitHash())
  
  def test(self):
    self.assertIsNone(naja.NLUniverse.get())
    naja.NLUniverse.create()
    self.assertIsNotNone(naja.NLUniverse.get())
    self.assertIsNone(naja.NLUniverse.get().getTopDesign())
    with self.assertRaises(RuntimeError) as context: naja.NLUniverse.create()

  def testDestroyTwice(self):
    universe = naja.NLUniverse.create()
    universe.destroy()

    with self.assertRaisesRegex(
        RuntimeError,
        r"applying a destroy\(\) to a Python object with no Hurricane object attached"):
      universe.destroy()

if __name__ == '__main__':
  unittest.main()
