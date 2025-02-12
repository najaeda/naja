# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import snl

def constructEquipotentialDesign(db):
  primitivesLib = snl.SNLLibrary.createPrimitives(db, "Primitives")
  designLib = snl.SNLLibrary.create(db, "DESIGN")

  #Primitive
  prim = snl.SNLDesign.createPrimitive(primitivesLib, "PRIM")
  primi = snl.SNLScalarTerm.create(prim, snl.SNLTerm.Direction.Input, "i")

  #Design AA
  aa = snl.SNLDesign.create(designLib, 'AA')
  aai = snl.SNLScalarTerm.create(aa, snl.SNLTerm.Direction.Input, 'i')
  aan = snl.SNLScalarNet.create(aa, 'n')
  aao = snl.SNLBusTerm.create(aa, 3, 3, snl.SNLTerm.Direction.Output, 'o')
  aai.setNet(aan)
  aao.getBusTermBit(3).setNet(aan)
  aaprim = snl.SNLInstance.create(aa, prim, "p")
  aaprim.getInstTerm(primi).setNet(aan)

  #Design A
  a = snl.SNLDesign.create(designLib, 'A')
  ai = snl.SNLScalarTerm.create(a, snl.SNLTerm.Direction.Input, 'i')
  ain = snl.SNLScalarNet.create(a, 'i')
  ai.setNet(ain)
  ao = snl.SNLScalarTerm.create(a, snl.SNLTerm.Direction.Output, 'o')
  aon = snl.SNLScalarNet.create(a, 'o')
  ao.setNet(aon)
  aaa = snl.SNLInstance.create(a, aa, "aa")
  aaa.getInstTerm(aai).setNet(ain)
  aaa.getInstTerm(aao.getBusTermBit(3)).setNet(aon)

  #Design BB
  bb = snl.SNLDesign.create(designLib, 'BB')
  bbi = snl.SNLScalarTerm.create(bb, snl.SNLTerm.Direction.Input, 'i')
  bbin = snl.SNLScalarNet.create(bb, 'i')
  bbi.setNet(bbin)
  bbprim = snl.SNLInstance.create(bb, prim, "p")
  bbprim.getInstTerm(primi).setNet(bbin)

  #Design B
  b = snl.SNLDesign.create(designLib, 'B')
  bi = snl.SNLScalarTerm.create(b, snl.SNLTerm.Direction.Input, 'i')
  bin = snl.SNLScalarNet.create(b, 'i')
  bi.setNet(bin)
  bbb = snl.SNLInstance.create(b, bb, "bb")
  bbb.getInstTerm(bbi).setNet(bin)

  #Design C
  c = snl.SNLDesign.create(designLib, 'C')
  ci = snl.SNLScalarTerm.create(c, snl.SNLTerm.Direction.Input, 'i')
  co = snl.SNLScalarTerm.create(c, snl.SNLTerm.Direction.Input, 'o')
  cn = snl.SNLScalarNet.create(c, 'n')
  ci.setNet(cn)
  co.setNet(cn)
  cprim = snl.SNLInstance.create(c, prim, "p")
  cprim.getInstTerm(primi).setNet(cn)

  #Top
  top = snl.SNLDesign.create(designLib, 'TOP')
  topi0 = snl.SNLScalarTerm.create(top, snl.SNLTerm.Direction.Input, 'i0')
  topi0n = snl.SNLScalarNet.create(top, 'i0')
  topi0.setNet(topi0n)
  topi1 = snl.SNLScalarTerm.create(top, snl.SNLTerm.Direction.Input, 'i1')
  topout = snl.SNLScalarTerm.create(top, snl.SNLTerm.Direction.Input, 'out')
  topoutn = snl.SNLScalarNet.create(top, 'out')
  topout.setNet(topoutn)
  a_ins = snl.SNLInstance.create(top, a, "a")
  b_ins = snl.SNLInstance.create(top, b, "b")
  c_ins = snl.SNLInstance.create(top, c, "c")
  topn = snl.SNLBusNet.create(top, -1, 5, 'n')
  topi1.setNet(topn.getBit(-1))
  a_ins.getInstTerm(ai).setNet(topi0n)
  a_ins.getInstTerm(ao).setNet(topn.getBit(-1))
  b_ins.getInstTerm(bi).setNet(topn.getBit(-1))
  c_ins.getInstTerm(ci).setNet(topn.getBit(-1))
  c_ins.getInstTerm(co).setNet(topoutn)

def constructDB(db):
  constructEquipotentialDesign(db)
