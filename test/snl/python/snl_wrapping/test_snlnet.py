# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl

class SNLNetTest(unittest.TestCase):
  def setUp(self):
    universe = snl.SNLUniverse.create()
    db = snl.SNLDB.create(universe)
    self.lib = snl.SNLLibrary.create(db)
    self.design = snl.SNLDesign.create(self.lib)

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test0(self):
    self.assertIsNotNone(self.design)

    i0 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Input, "I0")
    i1 = snl.SNLBusTerm.create(self.design, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    o = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Output, "O")

    self.assertEqual(3, sum(1 for t in self.design.getTerms()))
    self.assertEqual(0, sum(1 for n in self.design.getNets()))

    self.assertIsNone(i0.getNet())
    i0Net = snl.SNLScalarNet.create(self.design, "I0")
    self.assertIsNotNone(i0Net)
    self.assertEqual(self.design, i0Net.getDesign())
    self.assertEqual("I0", i0Net.getName())
    i0.setNet(i0Net)
    self.assertEqual(i0.getNet(), i0Net)
    self.assertEqual(i0Net, i0.getNet())
    self.assertEqual(i0Net, self.design.getNet("I0"))
    self.assertEqual(i0Net, self.design.getScalarNet("I0"))
    self.assertIsNone(self.design.getBusNet("I0"))
    self.assertEqual(1, sum(1 for b in i0Net.getBits()))
    self.assertEqual(1, sum(1 for n in self.design.getNets()))

    i1Net = snl.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertIsNotNone(i1Net)
    self.assertEqual("I1", i1Net.getName())
    self.assertEqual(4, i1.getMSB())
    self.assertEqual(0, i1.getLSB())
    self.assertEqual(5, i1.getSize())
    self.assertEqual(4, i1Net.getMSB())
    self.assertEqual(0, i1Net.getLSB())
    self.assertEqual(5, i1Net.getSize())
    self.assertEqual(i1Net, self.design.getNet("I1"))
    self.assertEqual(i1Net, self.design.getBusNet("I1"))
    self.assertIsNone(self.design.getScalarNet("I1"))
    i1.setNet(i1Net)
    self.assertEqual(5, sum(1 for b in i1Net.getBits()))
    self.assertEqual(2, sum(1 for n in self.design.getNets()))
    self.assertEqual(1+5, sum(1 for b in self.design.getBitNets()))
    
    i1Bit4 = i1.getBit(4)
    i1NetBit4 = i1Net.getBit(4)
    self.assertIsNotNone(i1Bit4)
    self.assertIsNotNone(i1NetBit4)
    self.assertEqual(i1Bit4.getNet(), i1NetBit4)

    self.assertIsNone(i1.getBit(5))
    self.assertIsNone(i1Net.getBit(5))

  def testRenameNet(self):
    i0Net = snl.SNLScalarNet.create(self.design, "I0")
    i1Net = snl.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertEqual("I0", i0Net.getName())
    self.assertEqual("I1", i1Net.getName())
    i0Net.setName("I2")
    i1Net.setName("I3")
    self.assertEqual("I2", i0Net.getName())
    self.assertEqual("I3", i1Net.getName())
    self.assertEqual(i0Net, self.design.getNet("I2"))
    self.assertEqual(i1Net, self.design.getNet("I3"))
    self.assertIsNone(self.design.getNet("I0"))
    self.assertIsNone(self.design.getNet("I1"))

  def testErrors(self):
    self.assertIsNotNone(self.design)
    i0 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Input, "I0")
    #wrong type
    with self.assertRaises(RuntimeError) as context: i0.setNet(self.design)
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarNet.create(self.lib, "I1")
    with self.assertRaises(RuntimeError) as context: snl.SNLBusNet.create(self.lib, 4, 0, "I1")
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarNet.create(self.design, 4, 0, "I1")
    with self.assertRaises(RuntimeError) as context: snl.SNLBusNet.create(self.lib, "I1")

    
if __name__ == '__main__':
  unittest.main()