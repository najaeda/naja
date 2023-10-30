import snl

def constructLUT4(lib):
  lut4 = snl.SNLDesign.createPrimitive(lib, "LUT4")
  i0 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I0")
  i1 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I1")
  i2 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I2")
  i3 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I3")
  o = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Output, "O")
  snl.SNLDesign.addCombinatorialArcs([i0,i1,i2,i3], o)

def constructPrimitives(lib):
  constructLUT4(lib)