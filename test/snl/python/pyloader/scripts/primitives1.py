# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import snl

def constructLOGIC0(lib):
  cell = snl.SNLDesign.createPrimitive(lib, 'LOGIC0')
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, 'Z')
  cell.setTruthTable(0b0)

def constructLOGIC1(lib):
  cell = snl.SNLDesign.createPrimitive(lib, 'LOGIC1')
  z = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, 'Z')
  cell.setTruthTable(0b1)

def constructAND2(lib):
  cell = snl.SNLDesign.createPrimitive(lib, 'AND2')
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, 'A1')
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, 'A2')
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, 'ZN')
  cell.setTruthTable(0x8)

def constructOR2(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "OR2")
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xE)

def constructOR3(lib):
  cell = snl.SNLDesign.createPrimitive(lib, "OR3")
  a1 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Input, "A3")
  zn = snl.SNLScalarTerm.create(cell, snl.SNLTerm.Direction.Output, "ZN")
  cell.setTruthTable(0xFE)

def constructPrimitives(lib):
  constructLOGIC0(lib)
  constructLOGIC1(lib)
  constructAND2(lib)
  constructOR2(lib)
  constructOR3(lib)