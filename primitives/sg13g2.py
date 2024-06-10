# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from naja import snl as snl

def constructsg13g2_inv(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_inv_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0b01)

def constructsg13g2_and2(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_and2_" + str(strength))
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0x8)

def constructsg13g2_and3(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_and3_" + str(strength))
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0x80)

def constructsg13g2_and4(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_and4_" + str(strength))
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0x8000)

def constructsg13g2_a21oi(lib, strengths):
  #function : "!((A1*A2)+B1)";
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_a21oi_" + str(strength))
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x07)

def constructsg13g2_a21o(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_a21o_" + str(strength))
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0xF8)

def constructsg13g2_a221oi(lib, strengths):
  #function : "!((A1*A2)+(B1*B2)+C1)";
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_a221oi_" + str(strength))
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x00000777)

def constructsg13g2_buf(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_buf_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0b10)

def constructsg13g2_dfrbp_1(lib):
  #function : "IQ";
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_dfrbp_1")
  clk = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CLK")
  reset_b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "RESET_B")
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  q = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Q")
  q_n = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Q_N")

def constructsg13g2_dlygate4sd2_1(lib):
  #function : "A";
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_dlygate4sd2_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
  cell.setTruthTable(0b10)

def constructsg13g2_dlygate4sd3_1(lib):
  #function : "A";
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_dlygate4sd3_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
  cell.setTruthTable(0b10)

def constructsg13g2_fill(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_fill_" + str(strength))

def constructsg13g2_mux2(lib, strengths):
  # function : "(!S*A0)+(S*A1)";
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_mux2_" + str(strength))
    a0 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A0")
    a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    s = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "S")
    x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0xCA)

def constructsg13g2_mux4_1(lib):
  # function : "(A0*(!S0*!S1))+(A1*(S0*!S1))+(A2*(!S0*S1))+(A3*(S0*S1))";
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_mux4_1")
  a0 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A0")
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  s0 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "S0")
  s1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "S1")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_nand2(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand2_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x7)

def constructsg13g2_nand3(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand3_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x7F)

def constructsg13g2_nand4(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand4_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x7FFF)

def constructsg13g2_nand2b(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand2b_" + str(strength))
    a_n = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A_N")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nand3b(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand3b_" + str(strength))
    a_n = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A_N")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nor2(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nor2_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x1)

def constructsg13g2_nor3(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nor3_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x01)

def constructsg13g2_nor4(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nor4_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x0001)

def constructsg13g2_nor2b(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nor2b_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b_n = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B_N")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_o21ai_1(lib):
  # function : "!((A1+A2)*B1)";
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_o21ai_1")
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_or2(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_or2_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0xE)

def constructsg13g2_or3(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_or3_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0xFE)

def constructsg13g2_or4(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_or4_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0xFFFE)

def constructsg13g2_slgcp_1(lib):
  # state_function : "CLK * int_GATE";
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_slgcp_1")
  gate = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "GATE")
  sce = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "SCE")
  clk = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CLK")
  gclk = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "GCLK")

def constructsg13g2_tiehi(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_tiehi")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "L_HI")
  cell.setTruthTable(0b1)

def constructsg13g2_tielo(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_tielo")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "L_LO")
  cell.setTruthTable(0b0)

def constructsg13g2_xor2(lib, strengths):
  #function : "(A^B)";
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_xor2_" + str(strength))
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")
    cell.setTruthTable(0x6)

def constructsg13g2_xnor2(lib, strengths):
  #function : "!(A^B)";
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_xnor2_" + str(strength))
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x9)

def constructPrimitives(lib):
  lib.setName('sg13g2')
  constructsg13g2_inv(lib, [1, 2])
  constructsg13g2_and2(lib, [1])
  constructsg13g2_and3(lib, [1])
  constructsg13g2_and4(lib, [1])
  constructsg13g2_a21o(lib, [1])
  constructsg13g2_a21oi(lib, [1, 2])
  constructsg13g2_a221oi(lib, [1])
  constructsg13g2_buf(lib, [1, 2, 4, 8])
  constructsg13g2_dfrbp_1(lib)
  constructsg13g2_dlygate4sd2_1(lib)
  constructsg13g2_dlygate4sd3_1(lib)
  constructsg13g2_fill(lib, [1, 2, 4, 8])
  constructsg13g2_mux2(lib, [1, 2])
  constructsg13g2_mux4_1(lib)
  constructsg13g2_nand2(lib, [1, 2])
  constructsg13g2_nand3(lib, [1])
  constructsg13g2_nand4(lib, [1])
  constructsg13g2_nand2b(lib, [1, 2])
  constructsg13g2_nand3b(lib, [1])
  constructsg13g2_nor2(lib, [1, 2])
  constructsg13g2_nor3(lib, [1, 2])
  constructsg13g2_nor2b(lib, [1, 2])
  constructsg13g2_nor4(lib, [1, 2])
  constructsg13g2_or2(lib, [1])
  constructsg13g2_or3(lib, [1])
  constructsg13g2_or4(lib, [1])
  constructsg13g2_o21ai_1(lib)
  constructsg13g2_slgcp_1(lib)
  constructsg13g2_tiehi(lib)
  constructsg13g2_tielo(lib)
  constructsg13g2_xor2(lib, [1])
  constructsg13g2_xnor2(lib, [1])
