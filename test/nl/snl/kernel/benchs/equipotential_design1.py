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
  aan = naja.SNLScalarNet.create(aa, 'n')
  aao = naja.SNLScalarTerm.create(aa, naja.SNLTerm.Direction.Output, 'o')
  aao.setNet(aan)
  aan.setType(naja.SNLNet.Type.Assign0)

  #Design A
  a = naja.SNLDesign.create(designLib, 'A')
  ao = naja.SNLScalarTerm.create(a, naja.SNLTerm.Direction.Output, 'o')
  aon = naja.SNLScalarNet.create(a, 'o')
  ao.setNet(aon)
  aaa = naja.SNLInstance.create(a, aa, "aa")
  aaa.getInstTerm(aao).setNet(aon)

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
  topout = naja.SNLScalarTerm.create(top, naja.SNLTerm.Direction.Output, 'out')
  topoutn = naja.SNLScalarNet.create(top, 'out')
  topout.setNet(topoutn)
  a_ins = naja.SNLInstance.create(top, a, "a")
  b_ins = naja.SNLInstance.create(top, b, "b")
  c_ins = naja.SNLInstance.create(top, c, "c")
  topn = naja.SNLScalarNet.create(top, 'n')
  a_ins.getInstTerm(ao).setNet(topn)
  b_ins.getInstTerm(bi).setNet(topn)
  c_ins.getInstTerm(ci).setNet(topn)
  c_ins.getInstTerm(co).setNet(topoutn)

def constructDB(db):
  constructEquipotentialDesign(db)
