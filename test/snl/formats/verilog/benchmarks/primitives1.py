import snl

def constructCFG2(lib):
  cfg2 = snl.SNLDesign.createPrimitive(lib, "CFG2")
  a = snl.SNLScalarTerm.create(cfg2, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cfg2, snl.SNLTerm.Direction.Input, "B")
  y = snl.SNLScalarTerm.create(cfg2, snl.SNLTerm.Direction.Output, "Y")

def constructCFG4(lib):
  cfg4 = snl.SNLDesign.createPrimitive(lib, "CFG4")
  a = snl.SNLScalarTerm.create(cfg4, snl.SNLTerm.Direction.Input, "A")
  b = snl.SNLScalarTerm.create(cfg4, snl.SNLTerm.Direction.Input, "B")
  c = snl.SNLScalarTerm.create(cfg4, snl.SNLTerm.Direction.Input, "C")
  d = snl.SNLScalarTerm.create(cfg4, snl.SNLTerm.Direction.Input, "D")
  y = snl.SNLScalarTerm.create(cfg4, snl.SNLTerm.Direction.Output, "Y")
  snl.SNLParameter.create_binary(cfg4, "INIT", 16, 0x0000)

def constructGND(lib):
  gnd = snl.SNLDesign.createPrimitive(lib, "GND")
  y = snl.SNLScalarTerm.create(gnd, snl.SNLTerm.Direction.Output, "Y")

def constructVCC(lib):
  vcc = snl.SNLDesign.createPrimitive(lib, "VCC")
  y = snl.SNLScalarTerm.create(vcc, snl.SNLTerm.Direction.Output, "Y")

def constructPrimitives(lib):
  constructCFG2(lib)
  constructCFG4(lib)
  constructGND(lib)
  constructVCC(lib)