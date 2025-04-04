import naja

def constructCFG2(lib):
  cfg2 = naja.SNLDesign.createPrimitive(lib, "CFG2")
  a = naja.SNLScalarTerm.create(cfg2, naja.SNLTerm.Direction.Input, "A")
  b = naja.SNLScalarTerm.create(cfg2, naja.SNLTerm.Direction.Input, "B")
  y = naja.SNLScalarTerm.create(cfg2, naja.SNLTerm.Direction.Output, "Y")

def constructCFG4(lib):
  cfg4 = naja.SNLDesign.createPrimitive(lib, "CFG4")
  a = naja.SNLScalarTerm.create(cfg4, naja.SNLTerm.Direction.Input, "A")
  b = naja.SNLScalarTerm.create(cfg4, naja.SNLTerm.Direction.Input, "B")
  c = naja.SNLScalarTerm.create(cfg4, naja.SNLTerm.Direction.Input, "C")
  d = naja.SNLScalarTerm.create(cfg4, naja.SNLTerm.Direction.Input, "D")
  y = naja.SNLScalarTerm.create(cfg4, naja.SNLTerm.Direction.Output, "Y")
  naja.SNLParameter.create_binary(cfg4, "INIT", 16, 0x0000)

def constructGND(lib):
  gnd = naja.SNLDesign.createPrimitive(lib, "GND")
  y = naja.SNLScalarTerm.create(gnd, naja.SNLTerm.Direction.Output, "Y")

def constructVCC(lib):
  vcc = naja.SNLDesign.createPrimitive(lib, "VCC")
  y = naja.SNLScalarTerm.create(vcc, naja.SNLTerm.Direction.Output, "Y")

def constructPrimitives(lib):
  constructCFG2(lib)
  constructCFG4(lib)
  constructGND(lib)
  constructVCC(lib)
