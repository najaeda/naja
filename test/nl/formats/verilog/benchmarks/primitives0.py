import naja

def constructLUT4(lib):
  lut4 = naja.SNLDesign.createPrimitive(lib, "LUT4")
  i0 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I0")
  i1 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I1")
  i2 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I2")
  i3 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I3")
  q = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Output, "Q")
  naja.SNLParameter.create_binary(lut4, "INIT", 16, 0x0000)

def constructPrimitives(lib):
  constructLUT4(lib)
