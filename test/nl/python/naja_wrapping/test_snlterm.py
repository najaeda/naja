# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja


class SNLTermResizeTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    self.lib = naja.NLLibrary.create(db)
    self.design = naja.SNLDesign.create(self.lib)

  def tearDown(self):
    del self.design
    del self.lib
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def test_bus_term_resize(self):
    term = naja.SNLBusTerm.create(self.design, naja.SNLTerm.Direction.InOut, 3, 0, "bus")
    term.setMSB(2)
    self.assertEqual(2, term.getMSB())
    self.assertEqual(0, term.getLSB())
    self.assertEqual(3, term.getWidth())
    term.setLSB(1)
    self.assertEqual(2, term.getMSB())
    self.assertEqual(1, term.getLSB())
    self.assertEqual(2, term.getWidth())
    term.setMSB(2)  # no-op
    term.setLSB(1)  # no-op

  def test_bus_term_resize_invalid(self):
    term = naja.SNLBusTerm.create(self.design, naja.SNLTerm.Direction.InOut, 3, 0, "bus")
    with self.assertRaises(RuntimeError):
      term.setMSB(-1)
    with self.assertRaises(RuntimeError):
      term.setLSB(4)




if __name__ == '__main__':
  unittest.main()
