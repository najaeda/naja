# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from naja import snl as snl

def constructsg13g2_inv(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_inv_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_and2_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_and2_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_and3_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_and3_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_and4_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_and4_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_a21oi(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_a21oi_" + str(strength))
    a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_a21o_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_a21o_1")
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_a221oi_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_a221oi_1")
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  b2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
  c1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
  y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_buf(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_buf_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_dfrbp_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_dfrbp_1")
  clk = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CLK")
  reset_b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "RESET_B")
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  q = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Q")
  q_n = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Q_N")

def constructsg13g2_dlygate4sd2_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_dlygate4sd2_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_dlygate4sd3_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_dlygate4sd3_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_fill(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_fill_" + str(strength))

def constructsg13g2_mux2(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_mux2_" + str(strength))
    a0 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A0")
    a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    s = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "S")
    x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_mux4_1(lib):
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

def constructsg13g2_nand2b(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand2b_" + str(strength))
    a_n = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A_N")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nand3_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand3_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
  y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nand3b_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand3b_1")
  a_n = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A_N")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
  y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nand4_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nand4_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nor2(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nor2_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nor2b(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nor2b_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b_n = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B_N")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nor3(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nor3_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_nor4(lib, strengths):
  for strength in strengths:
    cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_nor4_" + str(strength))
    a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_o21ai_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_o21ai_1")
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  b1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
  y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructsg13g2_or2_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_or2_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_or3_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_or3_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_or4_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_or4_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  c = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
  d = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_slgcp_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_slgcp_1")
  gate = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "GATE")
  sce = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "SCE")
  clk = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CLK")
  gclk = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "GCLK")

def constructsg13g2_tiehi(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_tiehi")
  l_lo = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "L_HI")

def constructsg13g2_tielo(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_tielo")
  l_lo = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "L_LO")

def constructsg13g2_xor2_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_xor2_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  x = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "X")

def constructsg13g2_xnor2_1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "sg13g2_xnor2_1")
  a = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  y = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")

def constructPrimitives(lib):
  lib.setName('sg13g2')
  constructsg13g2_inv(lib, [1, 2])
  constructsg13g2_and2_1(lib)
  constructsg13g2_and3_1(lib)
  constructsg13g2_and4_1(lib)
  constructsg13g2_a21o_1(lib)
  constructsg13g2_a21oi(lib, [1, 2])
  constructsg13g2_a221oi_1(lib)
  constructsg13g2_buf(lib, [1, 2, 4, 8])
  constructsg13g2_dfrbp_1(lib)
  constructsg13g2_dlygate4sd2_1(lib)
  constructsg13g2_dlygate4sd3_1(lib)
  constructsg13g2_fill(lib, [1, 2, 4, 8])
  constructsg13g2_mux2(lib, [1, 2])
  constructsg13g2_mux4_1(lib)
  constructsg13g2_nand2(lib, [1, 2])
  constructsg13g2_nand3_1(lib)
  constructsg13g2_nand4_1(lib)
  constructsg13g2_nand2b(lib, [1, 2])
  constructsg13g2_nand3b_1(lib)
  constructsg13g2_nor2(lib, [1, 2])
  constructsg13g2_nor3(lib, [1, 2])
  constructsg13g2_nor2b(lib, [1, 2])
  constructsg13g2_nor4(lib, [1, 2])
  constructsg13g2_or2_1(lib)
  constructsg13g2_or3_1(lib)
  constructsg13g2_or4_1(lib)
  constructsg13g2_o21ai_1(lib)
  constructsg13g2_slgcp_1(lib)
  constructsg13g2_tiehi(lib)
  constructsg13g2_tielo(lib)
  constructsg13g2_xor2_1(lib)
  constructsg13g2_xnor2_1(lib)
