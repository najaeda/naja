# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from naja import snl as snl

def constructBUF(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "BUF_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")

def constructCLKBUF(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "CLKBUF_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")

def constructINV(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "INV_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructLOGIC0(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "LOGIC0_X" + str(X))
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")

def constructLOGIC1(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "LOGIC1_X" + str(X))
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")

def constructAND2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "AND2_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructAND3(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "AND3_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructAND4(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "AND4_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  a4 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A4")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructAOI21(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "AOI21_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructDFF(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "DFF_X" + str(X))
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  ck = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CK")
  q = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Q")
  qn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "QN")

def constructAOI22(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "AOI22_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructFA(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "FA_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CI")
  co = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "CO")
  s = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "S")

def constructHA(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "HA_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  co = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "CO")
  s = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "S")

def constructMUX2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "MUX2_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  s = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "S")
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")

def constructNAND2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NAND2_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructNAND3(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NAND3_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  
def constructNAND4(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NAND4_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  a4 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A4")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructNOR2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NOR2_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructNOR3(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NOR3_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructNOR4(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NOR4_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A4")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOR2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OR2_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOR4(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OR4_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  a4 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A4")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOAI21(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OAI21_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOAI211(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OAI211_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  c2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOAI22(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OAI22_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOAI221(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OAI221_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  c1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  c2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOR3(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OR3_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructXOR2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "XOR2_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")

def constructXNOR2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "XNOR2_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructfakeram(lib, addr, data):
  cell = snl.SNLDesign.createPrimitive(lib, "fakeram45_" + str(pow(2, addr)) + "x" + str(data))
  clk = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "clk")
  we_in = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "we_in")
  ce_in = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "ce_in")
  wd_in = snl.SNLBusTerm.create(cell, snl.SNLTerm.Direction.Input, data-1, 0, "wd_in")
  w_mask_in = snl.SNLBusTerm.create(cell, snl.SNLTerm.Direction.Input, data-1, 0, "w_mask_in")
  addr_in = snl.SNLBusTerm.create(cell, snl.SNLTerm.Direction.Input, addr-1, 0, "addr_in")
  rd_out = snl.SNLBusTerm.create(cell, snl.SNLTerm.Direction.Output, data-1, 0, "rd_out")

def constructPrimitives(lib):
  constructBUF(lib, 1)
  constructBUF(lib, 2)
  constructBUF(lib, 4)
  constructBUF(lib, 8)
  constructBUF(lib, 16)
  constructBUF(lib, 32)
  constructCLKBUF(lib, 1)
  constructCLKBUF(lib, 2)
  constructCLKBUF(lib, 3)
  constructDFF(lib, 1)
  constructDFF(lib, 2)
  constructINV(lib, 1)
  constructINV(lib, 2)
  constructINV(lib, 4)
  constructINV(lib, 8)
  constructINV(lib, 16)
  constructINV(lib, 32)
  constructLOGIC0(lib, 1)
  constructLOGIC1(lib, 1)
  constructAND2(lib, 1)
  constructAND2(lib, 2)
  constructAND2(lib, 4)
  constructAND3(lib, 1)
  constructAND3(lib, 2)
  constructAND3(lib, 4)
  constructAND4(lib, 1)
  constructAND4(lib, 2)
  constructAND4(lib, 4)
  constructAOI21(lib, 1)
  constructAOI21(lib, 2)
  constructAOI21(lib, 4)
  constructAOI22(lib, 1)
  constructAOI22(lib, 2)
  constructAOI22(lib, 4)
  constructFA(lib, 1)
  constructHA(lib, 1)
  constructMUX2(lib, 1)
  constructMUX2(lib, 2)
  constructNAND2(lib, 1)
  constructNAND2(lib, 2)
  constructNAND2(lib, 4)
  constructNAND3(lib, 1)
  constructNAND3(lib, 2)
  constructNAND3(lib, 4)
  constructNAND4(lib, 1)
  constructNAND4(lib, 2)
  constructNAND4(lib, 4)
  constructNOR2(lib, 1)
  constructNOR2(lib, 2)
  constructNOR2(lib, 4)
  constructNOR3(lib, 1)
  constructNOR3(lib, 2)
  constructNOR3(lib, 4)
  constructNOR4(lib, 1)
  constructNOR4(lib, 2)
  constructNOR4(lib, 4)
  constructOR2(lib, 1)
  constructOR2(lib, 2)
  constructOR2(lib, 4)
  constructOR4(lib, 1)
  constructOR4(lib, 2)
  constructOR4(lib, 4)
  constructOAI21(lib, 1)
  constructOAI21(lib, 2)
  constructOAI21(lib, 4)
  constructOAI211(lib, 1)
  constructOAI211(lib, 2)
  constructOAI211(lib, 4)
  constructOAI22(lib, 1)
  constructOAI22(lib, 2)
  constructOAI22(lib, 4)
  constructOAI221(lib, 1)
  constructOAI221(lib, 2)
  constructOAI221(lib, 4)
  constructOR3(lib, 1)
  constructOR3(lib, 2)
  constructOR3(lib, 4)
  constructXOR2(lib, 1)
  constructXOR2(lib, 2)
  constructXNOR2(lib, 1)
  constructXNOR2(lib, 2)
  constructfakeram(lib, 6, 7)
  constructfakeram(lib, 6, 15)
  constructfakeram(lib, 6, 96)
  constructfakeram(lib, 9, 64)
  constructfakeram(lib, 8, 95)