# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from naja import snl as snl

def constructBUF(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "BUF_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")
  cell.setTruthTable(0b10)

def constructCLKBUF(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "CLKBUF_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")
  cell.setTruthTable(0b10)

def constructINV(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "INV_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0b01)

def constructLOGIC0(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "LOGIC0_X" + str(X))
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")
  cell.setTruthTable(0b0)

def constructLOGIC1(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "LOGIC1_X" + str(X))
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")
  cell.setTruthTable(0b1)

def constructAND2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "AND2_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x8)

def constructAND3(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "AND3_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x80)

def constructAND4(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "AND4_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  a4 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A4")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x8000)

def constructDFF(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "DFF_X" + str(X))
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  ck = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CK")
  q = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Q")
  qn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "QN")

def constructDFFR(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "DFFR_X" + str(X))
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  rn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "RN")
  ck = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CK")
  q = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Q")
  qn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "QN")

def constructDFFS(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "DFFS_X" + str(X))
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  sn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "SN")
  ck = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CK")
  q = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Q")
  qn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "QN")

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
  cell.setTruthTable(0xE4)

def constructNAND2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NAND2_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x7)

def constructNAND3(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NAND3_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x7F)
  
def constructNAND4(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NAND4_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  a4 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A4")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x7FFF)

def constructNOR2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NOR2_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x1)

def constructNOR3(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NOR3_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x01)

def constructNOR4(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "NOR4_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A4")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x0001)

def constructOR2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OR2_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xE)

def constructOR3(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OR3_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xFE)

def constructOR4(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "OR4_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  a4 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A4")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xFFFE)

def constructAOI21(lib, X):
  #function: !(A | (B1 & B2))
  cell = snl.SNLDesign.createPrimitive(lib, "AOI21_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xA8)

def constructAOI211(lib, X):
  #function: !(((C1 & C2) | B) | A)
  cell = snl.SNLDesign.createPrimitive(lib, "AOI211_X" + str(X))
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructAOI22(lib, X):
  #function: !((A1 & A2) | (B1 & B2));
  cell = snl.SNLDesign.createPrimitive(lib, "AOI22_X" + str(X))
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xEEE0)

def constructAOI221(lib, X):
  #function: !(((C1 & C2) | A) | (B1 & B2))
  cell = snl.SNLDesign.createPrimitive(lib, "AOI221_X" + str(X))
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructAOI222(lib, X):
  #function: !(((A1 & A2) | (B1 & B2)) | (C1 & C2))
  cell = snl.SNLDesign.createPrimitive(lib, "AOI222_X" + str(X))
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOAI21(lib, X):
  #function: !(A & (B1 | B2))
  cell = snl.SNLDesign.createPrimitive(lib, "OAI21_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x57)

def constructOAI211(lib, X):
  #function: !(((C1 | C2) & A) & B)
  cell = snl.SNLDesign.createPrimitive(lib, "OAI211_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  c2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xEEEF)

def constructOAI22(lib, X):
  #function: !((A1 | A2) & (B1 | B2))
  cell = snl.SNLDesign.createPrimitive(lib, "OAI22_X" + str(X))
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x111F)

def constructOAI221(lib, X):
  #function: !(((C1 | C2) & A) & (B1 | B2));
  cell = snl.SNLDesign.createPrimitive(lib, "OAI221_X" + str(X))
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xFF757575)

def constructOAI222(lib, X):
  #function: !(((A1 | A2) & (B1 | B2)) & (C1 | C2))
  cell = snl.SNLDesign.createPrimitive(lib, "OAI222_X" + str(X))
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")

def constructOAI33(lib, X):
  #function: !(((A1 | A2) | A3) & ((B1 | B2) | B3))
  cell = snl.SNLDesign.createPrimitive(lib, "OAI33_X" + str(X))
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B3")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")


def constructXOR2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "XOR2_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Z")
  cell.setTruthTable(0x6)

def constructXNOR2(lib, X):
  cell = snl.SNLDesign.createPrimitive(lib, "XNOR2_X" + str(X))
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x9)

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
  lib.setName('nangate45')
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
  constructDFFR(lib, 1)
  constructDFFS(lib, 1)
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
  constructAOI211(lib, 1)
  constructAOI211(lib, 2)
  constructAOI211(lib, 4)
  constructAOI22(lib, 1)
  constructAOI22(lib, 2)
  constructAOI22(lib, 4)
  constructAOI221(lib, 1)
  constructAOI221(lib, 2)
  constructAOI221(lib, 4)
  constructAOI222(lib, 1)
  constructAOI222(lib, 2)
  constructAOI222(lib, 4)
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
  constructOR3(lib, 1)
  constructOR3(lib, 2)
  constructOR3(lib, 4)
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
  constructOAI222(lib, 1)
  constructOAI33(lib, 1)
  constructXOR2(lib, 1)
  constructXOR2(lib, 2)
  constructXNOR2(lib, 1)
  constructXNOR2(lib, 2)
  constructfakeram(lib, 6, 7)
  constructfakeram(lib, 6, 15)
  constructfakeram(lib, 6, 21)
  constructfakeram(lib, 6, 96)
  constructfakeram(lib, 9, 64)
  constructfakeram(lib, 8, 95)
  constructfakeram(lib, 8, 34)
  constructfakeram(lib, 11, 39)
