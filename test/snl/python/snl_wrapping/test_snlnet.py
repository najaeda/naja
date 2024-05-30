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
    self.model = snl.SNLDesign.create(self.lib, "model")

  def tearDown(self):
    del self.lib
    del self.design
    del self.model
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
    self.assertEqual(snl.SNLNet.Type.Standard, i0Net.getType())
    i0.setNet(i0Net)
    self.assertEqual(i0.getNet(), i0Net)
    self.assertEqual(i0Net, i0.getNet())
    self.assertEqual(i0Net, self.design.getNet("I0"))
    self.assertEqual(i0Net, self.design.getScalarNet("I0"))
    self.assertIsNone(self.design.getBusNet("I0"))
    self.assertEqual(1, sum(1 for b in i0Net.getBits()))
    self.assertEqual(1, sum(1 for n in self.design.getNets()))
    self.assertEqual(1, sum(1 for n in self.design.getScalarNets()))
    self.assertEqual(0, sum(1 for n in self.design.getBusNets()))
    self.assertEqual(1, sum(1 for c in i0Net.getComponents())) 
    self.assertEqual(1, sum(1 for b in i0Net.getBitTerms())) 
    self.assertEqual(0, sum(1 for i in i0Net.getInstTerms()))
    c = next(iter(i0Net.getComponents()))
    self.assertEqual(i0, c)
    self.assertIsInstance(c, snl.SNLNetComponent)
    self.assertIsInstance(c, snl.SNLBitTerm)
    self.assertIsInstance(c, snl.SNLScalarTerm)

    i1Net = snl.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertIsInstance(i1Net, snl.SNLBusNet)
    self.assertIsInstance(i1Net, snl.SNLNet)
    self.assertIsInstance(i1Net, snl.SNLDesignObject)
    self.assertIsNotNone(i1Net)
    self.assertEqual("I1", i1Net.getName())
    with self.assertRaises(RuntimeError) as context: snl.SNLBusNet.create(self.design, 4, 0, "I1")
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
    with self.assertRaises(RuntimeError) as context: i1.setNet(self.design)
    self.assertEqual(5, sum(1 for b in i1Net.getBits()))
    self.assertEqual(2, sum(1 for n in self.design.getNets()))
    self.assertEqual(1, sum(1 for n in self.design.getScalarNets()))
    self.assertEqual(1, sum(1 for n in self.design.getBusNets()))
    self.assertEqual(1+5, sum(1 for b in self.design.getBitNets()))
    i1Bit4 = i1.getBit(4)
    
    i1Bit4 = i1.getBit(4)
    i1NetBit4 = i1Net.getBit(4)
    self.assertIsNotNone(i1Bit4)
    self.assertIsNotNone(i1NetBit4)
    self.assertEqual(i1Bit4.getNet(), i1NetBit4)
    self.assertEqual(1, sum(1 for c in i1NetBit4.getComponents()))
    self.assertEqual(1, sum(1 for b in i1NetBit4.getBitTerms()))
    self.assertEqual(0, sum(1 for i in i1NetBit4.getInstTerms()))
    c = next(iter(i1NetBit4.getComponents()))
    self.assertEqual(i1Bit4, c)
    self.assertIsInstance(c, snl.SNLNetComponent)
    self.assertIsInstance(c, snl.SNLBitTerm)
    self.assertIsInstance(c, snl.SNLBusTermBit)

    #create another design
    design2 = snl.SNLDesign.create(self.lib)
    design2Net = snl.SNLScalarNet.create(design2, "net")
    with self.assertRaises(RuntimeError) as context: i1.setNet(design2Net)

    self.assertIsNone(i1.getBit(5))
    self.assertIsNone(i1Net.getBit(5))
    del i1Bit4
    del i1NetBit4

  def testComponents(self):
    topI0 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Input, "I0")
    topI1 = snl.SNLBusTerm.create(self.design, snl.SNLTerm.Direction.Input, 4, 0, "I1")
    topO0 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Output, "O0")
    topO1 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Output, "O1")

    i0 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "i0")
    i1 = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Input, "i1")
    o = snl.SNLScalarTerm.create(self.model, snl.SNLTerm.Direction.Output, "o")

    ins1 = snl.SNLInstance.create(self.design, self.model, "ins1")
    ins2 = snl.SNLInstance.create(self.design, self.model, "ins2")

    i0Net = snl.SNLScalarNet.create(self.design, "I0")
    topI0.setNet(i0Net)
    i1Net = snl.SNLScalarNet.create(self.design, "I1")
    topI1.getBit(0).setNet(i1Net)
    o0Net = snl.SNLScalarNet.create(self.design, "O0")
    topO0.setNet(o0Net)
    o1Net = snl.SNLScalarNet.create(self.design, "O1")
    topO1.setNet(o1Net)
    self.assertIsNotNone(self.model.getScalarTerm("i0"))
    ins1I0 = ins1.getInstTerm(self.model.getScalarTerm("i0"))
    self.assertIsNotNone(ins1I0)
    ins1I0.setNet(i0Net)
    del ins1I0
    ins1.getInstTerm(self.model.getScalarTerm("i1")).setNet(i1Net)
    ins1.getInstTerm(self.model.getScalarTerm("o")).setNet(o0Net)
    ins2.getInstTerm(self.model.getScalarTerm("i0")).setNet(i0Net)
    ins2.getInstTerm(self.model.getScalarTerm("i1")).setNet(i1Net)
    ins2.getInstTerm(self.model.getScalarTerm("o")).setNet(o1Net)
    self.assertEqual(3, sum(1 for c in i0Net.getComponents()))
    i0NetList = list(i0Net.getComponents())
    self.assertEqual(3, len(i0NetList))
    self.assertEqual(topI0, i0NetList[0])
    self.assertEqual(ins1.getInstTerm(self.model.getScalarTerm("i0")), i0NetList[1])
    self.assertEqual(ins2.getInstTerm(self.model.getScalarTerm("i0")), i0NetList[2])

    i1NetList = list(i1Net.getComponents())
    self.assertEqual(3, len(i1NetList))
    self.assertEqual(topI1.getBit(0), i1NetList[0])
    self.assertEqual(topI1, topI1.getBit(0).getBus())
    self.assertEqual(0, topI1.getBit(0).getBit())
    self.assertEqual(ins1.getInstTerm(self.model.getScalarTerm("i1")), i1NetList[1])
    self.assertEqual(ins2.getInstTerm(self.model.getScalarTerm("i1")), i1NetList[2])

    self.assertEqual(3, sum(1 for c in i1Net.getComponents()))
    self.assertEqual(2, sum(1 for c in o0Net.getComponents()))
    self.assertEqual(2, sum(1 for c in o1Net.getComponents()))

    #delete topI0 as a bitTerm
    i0NetList[0].destroy()
    self.assertEqual(2, sum(1 for c in i0Net.getComponents()))

    #delete ins0
    ins1.destroy()
    self.assertEqual(1, sum(1 for c in i0Net.getComponents()))

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
    self.assertEqual(i1Net, i1Net.getBit(0).getBus())
    self.assertEqual(0, i1Net.getBit(0).getBit())

  def testNetType(self):
    i0Net = snl.SNLScalarNet.create(self.design, "I0")
    i1Net = snl.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertEqual(snl.SNLNet.Type.Standard, i0Net.getType())
    i0Net.setType(snl.SNLNet.Type.Assign0)
    self.assertEqual(snl.SNLNet.Type.Assign0, i0Net.getType())
    i0Net.setType(snl.SNLNet.Type.Assign1)
    self.assertEqual(snl.SNLNet.Type.Assign1, i0Net.getType())
    self.assertTrue(i0Net.isConstant1())
    self.assertTrue(i0Net.isConstant())
    i0Net.setType(snl.SNLNet.Type.Supply0)
    self.assertEqual(snl.SNLNet.Type.Supply0, i0Net.getType())
    self.assertTrue(i0Net.isConstant0())
    self.assertTrue(i0Net.isConstant())
    i0Net.setType(snl.SNLNet.Type.Supply1)
    self.assertEqual(snl.SNLNet.Type.Supply1, i0Net.getType())
    i0Net.setType(snl.SNLNet.Type.Standard)
    self.assertFalse(i0Net.isConstant0())
    self.assertFalse(i0Net.isConstant1())
    self.assertFalse(i0Net.isConstant())
    self.assertEqual(snl.SNLNet.Type.Standard, i0Net.getType())

  def testErrors(self):
    self.assertIsNotNone(self.design)
    i0 = snl.SNLScalarTerm.create(self.design, snl.SNLTerm.Direction.Input, "I0")
    #wrong arg type
    with self.assertRaises(RuntimeError) as context: i0.setNet(self.design)
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarNet.create(self.lib, "I1")
    with self.assertRaises(RuntimeError) as context: snl.SNLBusNet.create(self.lib, 4, 0, "I1")
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarNet.create(self.design, 4, 0, "I1")
    with self.assertRaises(RuntimeError) as context: snl.SNLBusNet.create(self.lib, "I1")

    net = snl.SNLScalarNet.create(self.design, "net")
    with self.assertRaises(RuntimeError) as context: net.setType(i0)

  def testNameClash(self):
    i0Net = snl.SNLScalarNet.create(self.design, "I0")
    with self.assertRaises(RuntimeError) as context: snl.SNLScalarNet.create(self.design, "I0")
    with self.assertRaises(RuntimeError) as context: snl.SNLBusNet.create(self.design, 4, 0, "I0")
    
if __name__ == '__main__':
  unittest.main()