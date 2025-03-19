# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import os
import shutil
import naja

class SNLDesignCloneTest2(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    primitives = naja.NLLibrary.createPrimitives(db)
    self.prim = naja.SNLDesign.createPrimitive(primitives, "PRIM")
    primi = naja.SNLScalarTerm.create(self.prim, naja.SNLTerm.Direction.Input, "I")
    primo = naja.SNLBusTerm.create(self.prim, 1, 0, naja.SNLTerm.Direction.Output, "O")
    lib = naja.NLLibrary.create(db)
    self.design = naja.SNLDesign.create(lib, "DESIGN")
    i0 = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Input, "I0")
    i0Net = naja.SNLScalarNet.create(self.design, "I0")
    i0.setNet(i0Net)
    i1 = naja.SNLBusTerm.create(self.design, naja.SNLTerm.Direction.Input, -5, 4, "I1")
    i1Net = naja.SNLBusNet.create(self.design, -5, 4, "I1")
    i1Net.setType(naja.SNLNet.Type.Assign0)
    i1.setNet(i1Net)
    o = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Output, "O")
    oNet = naja.SNLScalarNet.create(self.design, "O")
    o.setNet(oNet)
    ins0 = naja.SNLInstance.create(self.design, self.prim, "INS0")
    ins0.getInstTerm(ins0.getModel().getScalarTerm("I")).setNet(i0Net)
    n0 = naja.SNLBusNet.create(self.design, 1, 0, "N0")
    n0.setType(naja.SNLNet.Type.Assign1)
    ins0.getInstTerm(ins0.getModel().getBusTerm("O").getBusTermBit(1)).setNet(n0.getBit(0))
    ins1 = naja.SNLInstance.create(self.design, self.prim, "INS1")
    ins1.getInstTerm(ins1.getModel().getScalarTerm("I")).setNet(n0.getBit(0))

  def tearDown(self):
    del self.design
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testCloneInterface(self):
    newDesign = self.design.clone()
    self.assertIsNotNone(newDesign)
    self.assertNotEqual(self.design, newDesign)
    self.assertTrue(newDesign.isAnonymous())
    self.assertEqual(self.design.getLibrary(), newDesign.getLibrary())
    self.assertEqual(self.design.getDB(), newDesign.getDB())
    termsSize = sum(1 for t in self.design.getTerms())
    newTermsSize = sum(1 for t in newDesign.getTerms())
    self.assertEqual(termsSize, newTermsSize)
    parametersSize = sum(1 for p in self.design.getParameters())
    newParametersSize = sum(1 for p in newDesign.getParameters())
    self.assertEqual(parametersSize, newParametersSize)

  def testClone(self):
    newDesign = self.design.clone("cloned")
    self.assertIsNotNone(newDesign)
    bench_dir = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(bench_dir)
    bench_dir = os.path.join(bench_dir, "najadesign_clone2")
    if os.path.exists(bench_dir):
      shutil.rmtree(bench_dir)
    os.makedirs(bench_dir)
    newDesign.dumpVerilog(os.path.join(bench_dir), "cloned.v")

    n0 = newDesign.getNet("N0")
    self.assertIsNotNone(n0)
    self.assertIsInstance(n0, naja.SNLBusNet)
    self.assertEqual(1, n0.getMSB())
    self.assertEqual(0, n0.getLSB())
    self.assertEqual(2, n0.getWidth())
    self.assertEqual(2, sum(1 for b in n0.getBits()))
    n0b0 = n0.getBit(0)
    self.assertIsNotNone(n0b0)
    self.assertIsInstance(n0b0, naja.SNLBusNetBit)
    self.assertEqual(naja.SNLNet.Type.Assign1, n0b0.getType())
    self.assertEqual(0, n0b0.getBit())
    self.assertEqual(n0, n0b0.getBus())
    self.assertEqual(2, sum(1 for c in n0b0.getComponents()))

if __name__ == '__main__':
  unittest.main()
