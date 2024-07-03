# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

#new "manual" implementation of asap7 primitives
#containing cells truth tables

from naja import snl as snl

def constructAND2(lib, strengths):
  for strength in strengths:
    name = 'AND2x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x8)

def constructAND3(lib, strengths):
  for strength in strengths:
    name = 'AND3x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x80)

def constructAND4(lib, strengths):
  for strength in strengths:
    name = 'AND4x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x8000)

def constructAND5(lib, strengths):
  for strength in strengths:
    name = 'AND5x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "E")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x80000000)

def constructAO21(lib, strengths):
  #function : "(A1 * A2) + (B)";
  for strength in strengths:
    name = 'AO21x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xF8)

def constructAO211(lib, strengths):
  #function : "(A1 * A2) + (B) + (C)";
  for strength in strengths:
    name = 'AO211x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xFFF8)

def constructAO22(lib, strengths):
  #function : "(A1 * A2) + (B1 * B2)";
  for strength in strengths:
    name = 'AO22x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xF888)

def constructAO221(lib, strengths):
  #function : "(A1 * A2) + (B1 * B2) + (C)";
  for strength in strengths:
    name = 'AO221x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xFFFFF888)

def constructAO222(lib, strengths):
  #function : "(A1 * A2) + (B1 * B2) + (C1 * C2)";
  for strength in strengths:
    name = 'AO222x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xFFFFF888F888F888)

def constructAO31(lib, strengths):
  #function : "(A1 * A2 * A3) + (B)";
  for strength in strengths:
    name = 'AO31x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xFF80)

def constructAO32(lib, strengths):
  #function : "(A1 * A2 * A3) + (B1 * B2)";
  for strength in strengths:
    name = 'AO32x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xFF808080)

def constructAO33(lib, strengths):
  #function : "(A1 * A2 * A3) + (B1 * B2 * B3)";
  for strength in strengths:
    name = 'AO33x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B3")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xff80808080808080)

def constructAOI21(lib, strengths):
  #function : "(!A1 * !B) + (!A2 * !B)";
  for strength in strengths:
    name = 'AOI21x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x07)

def constructAOI211(lib, strengths):
  #function : "(!A1 * !B * !C) + (!A2 * !B * !C)";
  for strength in strengths:
    name = 'AOI211x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x0007)

def constructAOI22(lib, strengths):
  #function : "(!A1 * !B1) + (!A1 * !B2) + (!A2 * !B1) + (!A2 * !B2)";
  for strength in strengths:
    name = 'AOI22x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x0777)

def constructAOI221(lib, strengths):
  #function : "(!A1 * !B1 * !C) + (!A1 * !B2 * !C) + (!A2 * !B1 * !C) + (!A2 * !B2 * !C)";
  for strength in strengths:
    name = 'AOI221x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x00000777)

def constructBUF(lib, strengths):
  for strength in strengths:
    name = 'BUFx' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x2)

def constructBUFf(lib, strengths):
  for strength in strengths:
    name = 'BUFx' + str(strength) + 'f_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x2)

def constructDFFHQN(lib, strengths):
  for strength in strengths:
    name = 'DFFHQNx' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CLK")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "QN")

def constructFAx1(lib):
  name = 'FAx1_ASAP7_75t_R'
  cell = snl.SNLDesign.createPrimitive(lib, name)
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "CI")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "CON")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "SN")

def constructHAxp5(lib):
  name = 'HAxp5_ASAP7_75t_R'
  cell = snl.SNLDesign.createPrimitive(lib, name)
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "CON")
  snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "SN")

def constructINV(lib, strengths):
  for strength in strengths:
    name = 'INVx' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x1)

def constructMAJ(lib, strengths):
  #function : "(A * B) + (A * C) + (B * C)";
  for strength in strengths:
    name = 'MAJx' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xe8)

def constructNAND2(lib, strengths):
  for strength in strengths:
    name = 'NAND2x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x7)

def constructNAND3(lib, strengths):
  for strength in strengths:
    name = 'NAND3x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x7F)

def constructNOR2(lib, strengths):
  for strength in strengths:
    name = 'NOR2x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x1)

def constructNOR3(lib, strengths):
  for strength in strengths:
    name = 'NOR3x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x01)

def constructOA21(lib, strengths):
  #function : "(A1 * B) + (A2 * B)";
  for strength in strengths:
    name = 'OA21x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xe0)

def constructOA211(lib, strengths):
  #function : "(A1 * B * C) + (A2 * B * C)";
  for strength in strengths:
    name = 'OA211x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xe000)

def constructOA22(lib, strengths):
  #function : "(A1 * B1) + (A1 * B2) + (A2 * B1) + (A2 * B2)";
  for strength in strengths:
    name = 'OA22x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xeee0)

def constructOA221(lib, strengths):
  #function : "(A1 * B1 * C) + (A1 * B2 * C) + (A2 * B1 * C) + (A2 * B2 * C)";
  for strength in strengths:
    name = 'OA221x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xeee00000)

def constructOA222(lib, strengths):
  #function : "(A1 * B1 * C1) + (A1 * B1 * C2) + (A1 * B2 * C1) + (A1 * B2 * C2) + (A2 * B1 * C1) + (A2 * B1 * C2) + (A2 * B2 * C1) + (A2 * B2 * C2)";
  for strength in strengths:
    name = 'OA222x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xeee0eee0eee00000)

def constructOA31(lib, strengths):
  #function : "(A1 * B1) + (A2 * B1) + (A3 * B1)";
  for strength in strengths:
    name = 'OA31x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xf0)

def constructOA33(lib, strengths):
  #function : "(A1 * B1) + (A1 * B2) + (A1 * B3) + (A2 * B1) + (A2 * B2) + (A2 * B3) + (A3 * B1) + (A3 * B2) + (A3 * B3)";
  for strength in strengths:
    name = 'OA33x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B3")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xfefefefefefefe00)

def constructOAI21(lib, strengths):
  #function : "(!A1 * !A2) + (!B)";
  for strength in strengths:
    name = 'OAI21x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x1f)

def constructOAI22(lib, strengths):
  #function : "(!A1 * !A2) + (!B1 * !B2)";
  for strength in strengths:
    name = 'OAI22x' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B1")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B2")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x111F)

def constructOR2(lib, strengths):
  for strength in strengths:
    name = 'OR2x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xE)

def constructOR3(lib, strengths):
  for strength in strengths:
    name = 'OR3x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xFE)

def constructOR4(lib, strengths):
    for strength in strengths:
      name = 'OR4x' + str(strength) + '_ASAP7_75t_R' 
      cell = snl.SNLDesign.createPrimitive(lib, name)
      snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
      snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
      snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
      snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
      snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
      cell.setTruthTable(0xFFFE)

def constructOR5(lib, strengths):
  for strength in strengths:
    name = 'OR5x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "C")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "D")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "E")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0xFFFFFFFE)

def constructTIEHI(lib, strengths):
  for strength in strengths:
    name = 'TIEHIx' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "H")
    cell.setTruthTable(0x1)

def constructTIELO(lib, strengths):
  for strength in strengths:
    name = 'TIELOx' + str(strength) + '_ASAP7_75t_R'
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "L")
    cell.setTruthTable(0x0)

def constructXOR2(lib, strengths):
  for strength in strengths:
    name = 'XOR2x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x6)

def constructXNOR2(lib, strengths):
  for strength in strengths:
    name = 'XNOR2x' + str(strength) + '_ASAP7_75t_R' 
    cell = snl.SNLDesign.createPrimitive(lib, name)
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "B")
    snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "Y")
    cell.setTruthTable(0x9)

def constructPrimitives(lib):
  lib.setName('asap7')
  constructAND2(lib, [2, 4])
  constructAND3(lib, [1, 2, 4]) 
  constructAND4(lib, [1, 2, 4]) 
  constructAND5(lib, [1, 2]) 
  constructAO21(lib, [1, 2])
  constructAO211(lib, [2])
  constructAO22(lib, [1, 2])
  constructAO221(lib, [1, 2])
  constructAO222(lib, [2])
  constructAO31(lib, [1, 2])
  constructAO32(lib, [1, 2])
  constructAO33(lib, [2])
  constructAOI21(lib, [1])
  constructAOI211(lib, [1])
  constructAOI22(lib, [1])
  constructAOI221(lib, [1])
  constructBUF(lib, [2, 3, 4, 5, 8])
  constructBUFf(lib, [2, 3, 4, 6, 12])
  constructDFFHQN(lib, [1])
  constructFAx1(lib)
  constructHAxp5(lib)
  constructINV(lib, [1, 2, 3, 4, 5, 8])
  constructMAJ(lib, [2])
  constructNAND2(lib, [1, 2])
  constructNAND3(lib, [1, 2])
  constructNOR2(lib, [1, 2])
  constructNOR3(lib, [1, 2])
  constructOA21(lib, [2])
  constructOA211(lib, [2])
  constructOA22(lib, [2])
  constructOA221(lib, [2])
  constructOA222(lib, [2])
  constructOA31(lib, [2])
  constructOA33(lib, [2])
  constructOAI21(lib, [1, 2])
  constructOAI22(lib, [1])
  constructOR2(lib, [2, 4, 6])
  constructOR3(lib, [1, 2])
  constructOR4(lib, [1, 2])
  constructOR5(lib, [1, 2])
  constructTIEHI(lib, [1])
  constructTIELO(lib, [1])
  constructXOR2(lib, [1, 2])
  constructXNOR2(lib, [1, 2])