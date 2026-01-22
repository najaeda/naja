# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import naja

def constructEquipotentialDesign(db):
  primitivesLib = naja.NLLibrary.createPrimitives(db, "Primitives")
  designLib = naja.NLLibrary.create(db, "DESIGN")

  #Primitive
  prim = naja.SNLDesign.createPrimitive(primitivesLib, "PRIM")
  primi = naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "i")

  #Design AA
  aa = naja.SNLDesign.create(designLib, 'AA')
  aai = naja.SNLScalarTerm.create(aa, naja.SNLTerm.Direction.Input, 'i')
  aan = naja.SNLScalarNet.create(aa, 'n')
  aao = naja.SNLBusTerm.create(aa, 3, 3, naja.SNLTerm.Direction.Output, 'o')
  aai.setNet(aan)
  aao.getBusTermBit(3).setNet(aan)
  aaprim = naja.SNLInstance.create(aa, prim, "p")
  aaprim.getInstTerm(primi).setNet(aan)

  #Design A
  a = naja.SNLDesign.create(designLib, 'A')
  ai = naja.SNLScalarTerm.create(a, naja.SNLTerm.Direction.Input, 'i')
  ain = naja.SNLScalarNet.create(a, 'i')
  ai.setNet(ain)
  ao = naja.SNLScalarTerm.create(a, naja.SNLTerm.Direction.Output, 'o')
  aon = naja.SNLScalarNet.create(a, 'o')
  ao.setNet(aon)
  aaa = naja.SNLInstance.create(a, aa, "aa")
  aaa.getInstTerm(aai).setNet(ain)
  aaa.getInstTerm(aao.getBusTermBit(3)).setNet(aon)

  #Design BB
  bb = naja.SNLDesign.create(designLib, 'BB')
  bbi = naja.SNLScalarTerm.create(bb, naja.SNLTerm.Direction.Input, 'i')
  bbin = naja.SNLScalarNet.create(bb, 'i')
  bbi.setNet(bbin)
  bbprim = naja.SNLInstance.create(bb, prim, "p")
  bbprim.getInstTerm(primi).setNet(bbin)

  #Design B
  b = naja.SNLDesign.create(designLib, 'B')
  bi = naja.SNLScalarTerm.create(b, naja.SNLTerm.Direction.Input, 'i')
  bin = naja.SNLScalarNet.create(b, 'i')
  bi.setNet(bin)
  bbb = naja.SNLInstance.create(b, bb, "bb")
  bbb.getInstTerm(bbi).setNet(bin)

  #Design C
  c = naja.SNLDesign.create(designLib, 'C')
  ci = naja.SNLScalarTerm.create(c, naja.SNLTerm.Direction.Input, 'i')
  co = naja.SNLScalarTerm.create(c, naja.SNLTerm.Direction.Input, 'o')
  cn = naja.SNLScalarNet.create(c, 'n')
  ci.setNet(cn)
  co.setNet(cn)
  cprim = naja.SNLInstance.create(c, prim, "p")
  cprim.getInstTerm(primi).setNet(cn)

  #Top
  top = naja.SNLDesign.create(designLib, 'TOP')
  topi0 = naja.SNLScalarTerm.create(top, naja.SNLTerm.Direction.Input, 'i0')
  topi0n = naja.SNLScalarNet.create(top, 'i0')
  topi0.setNet(topi0n)
  topi1 = naja.SNLScalarTerm.create(top, naja.SNLTerm.Direction.Input, 'i1')
  topout = naja.SNLScalarTerm.create(top, naja.SNLTerm.Direction.Output, 'out')
  topoutn = naja.SNLScalarNet.create(top, 'out')
  topout.setNet(topoutn)
  a_ins = naja.SNLInstance.create(top, a, "a")
  b_ins = naja.SNLInstance.create(top, b, "b")
  c_ins = naja.SNLInstance.create(top, c, "c")
  topn = naja.SNLBusNet.create(top, -1, 5, 'n')
  topi1.setNet(topn.getBit(-1))
  a_ins.getInstTerm(ai).setNet(topi0n)
  a_ins.getInstTerm(ao).setNet(topn.getBit(-1))
  b_ins.getInstTerm(bi).setNet(topn.getBit(-1))
  c_ins.getInstTerm(ci).setNet(topn.getBit(-1))
  c_ins.getInstTerm(co).setNet(topoutn)

def constructDB(db):
  constructEquipotentialDesign(db)
