# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import naja

def constructLUT4(lib):
  lut4 = naja.SNLDesign.createPrimitive(lib, "LUT4")
  i0 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I0")
  i1 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I1")
  i2 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I2")
  i3 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I3")
  o = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Output, "O")
  naja.SNLDesign.addCombinatorialArcs([i0,i1,i2,i3], o)

def constructPrimitives(lib):
  lib.setName('PRIMITIVES0')
  constructLUT4(lib)