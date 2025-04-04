# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import naja

def constructLOGIC0(lib):
  cell = naja.SNLDesign.createPrimitive(lib, 'LOGIC0')
  z = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, 'Z')
  cell.setTruthTable(0b0)

def constructLOGIC1(lib):
  cell = naja.SNLDesign.createPrimitive(lib, 'LOGIC1')
  z = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, 'Z')
  cell.setTruthTable(0b1)

def constructBUF(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "BUF")
  a = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A")
  z = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "Z")
  cell.setTruthTable(0b10)

def constructINV(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "INV")
  a = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A")
  zn = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0b01)

def constructAND2(lib):
  cell = naja.SNLDesign.createPrimitive(lib, 'AND2')
  a1 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, 'A1')
  a2 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, 'A2')
  zn = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, 'ZN')
  cell.setTruthTable(0x8)

def constructOR2(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "OR2")
  a1 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A1")
  a2 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A2")
  zn = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xE)

def constructOR3(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "OR3")
  a1 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A1")
  a2 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A2")
  a3 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A3")
  zn = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xFE)

def constructOR4(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "OR4")
  a1 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A1")
  a2 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A2")
  a3 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A3")
  a4 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A4")
  zn = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xFFFE)

def constructNOR2(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "NOR2")
  a1 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A1")
  a2 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A2")
  zn = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x1)

def constructXOR2(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "XOR2")
  a = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A")
  b = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "B")
  z = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "Z")
  cell.setTruthTable(0x6)

def constructXNOR2(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "XNOR2")
  a = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A")
  b = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "B")
  zn = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x9)

def constructOAI21(lib):
  #function: "!(A & (B1 | B2))";
  cell = naja.SNLDesign.createPrimitive(lib, "OAI21")
  a = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A")
  b1 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "B1")
  b2 = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "B2")
  zn = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0x57)

def constructMUX2(lib):
  cell = naja.SNLDesign.createPrimitive(lib, "MUX2")
  a = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "A")
  b = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "B")
  s = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Input, "S")
  z = naja.SNLScalarTerm.create(cell, naja.SNLTerm.Direction.Output, "Z")
  cell.setTruthTable(0xCA)

def constructPrimitives(lib):
  constructLOGIC0(lib)
  constructLOGIC1(lib)
  constructBUF(lib)
  constructINV(lib)
  constructAND2(lib)
  constructOR2(lib)
  constructOR3(lib)
  constructOR4(lib)
  constructNOR2(lib)
  constructXOR2(lib)
  constructXNOR2(lib)
  constructOAI21(lib)
  constructMUX2(lib)