# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLUniverseTest(unittest.TestCase):
  def tearDown(self):
    if snl.NLUniverse.get():
      snl.NLUniverse.get().destroy()

  def testVersion(self):
    self.assertIsNotNone(snl.getVersion())
    self.assertIsNotNone(snl.getGitVersion())
  
  def test(self):
    self.assertIsNone(snl.NLUniverse.get())
    snl.NLUniverse.create()
    self.assertIsNotNone(snl.NLUniverse.get())
    self.assertIsNone(snl.NLUniverse.get().getTopDesign())
    with self.assertRaises(RuntimeError) as context: snl.NLUniverse.create()

if __name__ == '__main__':
  unittest.main()
