import snl

def constructIBUF(lib):
  ibuf = snl.SNLDesign.createPrimitive(lib, "IBUF")
  i = snl.SNLScalarTerm.create(ibuf, snl.SNLTerm.Direction.Input, "I")
  o = snl.SNLScalarTerm.create(ibuf, snl.SNLTerm.Direction.Output, "O")

def constructOBUF(lib):
  obuf = snl.SNLDesign.createPrimitive(lib, "OBUF")
  i = snl.SNLScalarTerm.create(obuf, snl.SNLTerm.Direction.Input, "I")
  o = snl.SNLScalarTerm.create(obuf, snl.SNLTerm.Direction.Output, "O")

def constructBUFG(lib):
  bufg = snl.SNLDesign.createPrimitive(lib, "BUFG")
  i = snl.SNLScalarTerm.create(bufg, snl.SNLTerm.Direction.Input, "I")
  o = snl.SNLScalarTerm.create(bufg, snl.SNLTerm.Direction.Output, "O")

def constructINV(lib):
  inv = snl.SNLDesign.createPrimitive(lib, "INV")
  i = snl.SNLScalarTerm.create(inv, snl.SNLTerm.Direction.Input, "I")
  o = snl.SNLScalarTerm.create(inv, snl.SNLTerm.Direction.Output, "O")

def constructCARRY4(lib):
  carry4 = snl.SNLDesign.createPrimitive(lib, "CARRY4")
  o = snl.SNLBusTerm.create(carry4, snl.SNLTerm.Direction.Output, 3, 0, "O")
  co = snl.SNLBusTerm.create(carry4, snl.SNLTerm.Direction.Output, 3, 0, "CO")
  di = snl.SNLBusTerm.create(carry4, snl.SNLTerm.Direction.Input, 3, 0, "DI")
  s = snl.SNLBusTerm.create(carry4, snl.SNLTerm.Direction.Input, 3, 0, "S")
  cyinit = snl.SNLScalarTerm.create(carry4, snl.SNLTerm.Direction.Input, "CYINIT")
  ci = snl.SNLScalarTerm.create(carry4, snl.SNLTerm.Direction.Input, "CI")

def constructLUT1(lib):
  lut1 = snl.SNLDesign.createPrimitive(lib, "LUT1")
  i0 = snl.SNLScalarTerm.create(lut1, snl.SNLTerm.Direction.Input, "I0")
  o = snl.SNLScalarTerm.create(lut1, snl.SNLTerm.Direction.Output, "O")
  snl.SNLParameter.create(lut1, "INIT", "")

def constructLUT2(lib):
  lut2 = snl.SNLDesign.createPrimitive(lib, "LUT2")
  i0 = snl.SNLScalarTerm.create(lut2, snl.SNLTerm.Direction.Input, "I0")
  i1 = snl.SNLScalarTerm.create(lut2, snl.SNLTerm.Direction.Input, "I1")
  o = snl.SNLScalarTerm.create(lut2, snl.SNLTerm.Direction.Output, "O")
  snl.SNLParameter.create(lut2, "INIT", "")

def constructLUT3(lib):
  lut3 = snl.SNLDesign.createPrimitive(lib, "LUT3")
  i0 = snl.SNLScalarTerm.create(lut3, snl.SNLTerm.Direction.Input, "I0")
  i1 = snl.SNLScalarTerm.create(lut3, snl.SNLTerm.Direction.Input, "I1")
  i2 = snl.SNLScalarTerm.create(lut3, snl.SNLTerm.Direction.Input, "I2")
  o = snl.SNLScalarTerm.create(lut3, snl.SNLTerm.Direction.Output, "O")
  snl.SNLParameter.create(lut3, "INIT", "")

def constructLUT4(lib):
  lut4 = snl.SNLDesign.createPrimitive(lib, "LUT4")
  i0 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I0")
  i1 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I1")
  i2 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I2")
  i3 = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Input, "I3")
  o = snl.SNLScalarTerm.create(lut4, snl.SNLTerm.Direction.Output, "O")
  snl.SNLParameter.create(lut4, "INIT", "")

def constructLUT5(lib):
  lut5 = snl.SNLDesign.createPrimitive(lib, "LUT5")
  i0 = snl.SNLScalarTerm.create(lut5, snl.SNLTerm.Direction.Input, "I0")
  i1 = snl.SNLScalarTerm.create(lut5, snl.SNLTerm.Direction.Input, "I1")
  i2 = snl.SNLScalarTerm.create(lut5, snl.SNLTerm.Direction.Input, "I2")
  i3 = snl.SNLScalarTerm.create(lut5, snl.SNLTerm.Direction.Input, "I3")
  i4 = snl.SNLScalarTerm.create(lut5, snl.SNLTerm.Direction.Input, "I4")
  o = snl.SNLScalarTerm.create(lut5, snl.SNLTerm.Direction.Output, "O")
  snl.SNLParameter.create(lut5, "INIT", "")

def constructLUT6(lib):
  lut6 = snl.SNLDesign.createPrimitive(lib, "LUT6")
  i0 = snl.SNLScalarTerm.create(lut6, snl.SNLTerm.Direction.Input, "I0")
  i1 = snl.SNLScalarTerm.create(lut6, snl.SNLTerm.Direction.Input, "I1")
  i2 = snl.SNLScalarTerm.create(lut6, snl.SNLTerm.Direction.Input, "I2")
  i3 = snl.SNLScalarTerm.create(lut6, snl.SNLTerm.Direction.Input, "I3")
  i4 = snl.SNLScalarTerm.create(lut6, snl.SNLTerm.Direction.Input, "I4")
  i5 = snl.SNLScalarTerm.create(lut6, snl.SNLTerm.Direction.Input, "I5")
  o = snl.SNLScalarTerm.create(lut6, snl.SNLTerm.Direction.Output, "O")
  snl.SNLParameter.create(lut6, "INIT", "")

def constructMUXF7(lib):
  muxf7 = snl.SNLDesign.createPrimitive(lib, "MUXF7")
  i0 = snl.SNLScalarTerm.create(muxf7, snl.SNLTerm.Direction.Input, "I0")
  i1 = snl.SNLScalarTerm.create(muxf7, snl.SNLTerm.Direction.Input, "I1")
  o = snl.SNLScalarTerm.create(muxf7, snl.SNLTerm.Direction.Input, "O")
  s = snl.SNLScalarTerm.create(muxf7, snl.SNLTerm.Direction.Input, "S")

def constructMUXF8(lib):
  muxf8 = snl.SNLDesign.createPrimitive(lib, "MUXF8")
  i0 = snl.SNLScalarTerm.create(muxf8, snl.SNLTerm.Direction.Input, "I0")
  i1 = snl.SNLScalarTerm.create(muxf8, snl.SNLTerm.Direction.Input, "I1")
  o = snl.SNLScalarTerm.create(muxf8, snl.SNLTerm.Direction.Output, "O")
  s = snl.SNLScalarTerm.create(muxf8, snl.SNLTerm.Direction.Output, "S")

def constructSRL16E(lib):
  srl16e = snl.SNLDesign.createPrimitive(lib, "SRL16E")
  ce = snl.SNLScalarTerm.create(srl16e, snl.SNLTerm.Direction.Input, "CE")
  clk = snl.SNLScalarTerm.create(srl16e, snl.SNLTerm.Direction.Input, "CLK")
  d = snl.SNLScalarTerm.create(srl16e, snl.SNLTerm.Direction.Input, "D")
  q = snl.SNLScalarTerm.create(srl16e, snl.SNLTerm.Direction.Output, "Q")
  a0 = snl.SNLScalarTerm.create(srl16e, snl.SNLTerm.Direction.Input, "A0")
  a1 = snl.SNLScalarTerm.create(srl16e, snl.SNLTerm.Direction.Input, "A1")
  a2 = snl.SNLScalarTerm.create(srl16e, snl.SNLTerm.Direction.Input, "A2")
  a3 = snl.SNLScalarTerm.create(srl16e, snl.SNLTerm.Direction.Input, "A3")
  snl.SNLParameter.create(srl16e, "INIT", "")
  snl.SNLParameter.create(srl16e, "IS_CLK_INVERTED", "")

def constructFDRE(lib):
  fdre = snl.SNLDesign.createPrimitive(lib, "FDRE")
  q = snl.SNLScalarTerm.create(fdre, snl.SNLTerm.Direction.Output, "Q")
  c = snl.SNLScalarTerm.create(fdre, snl.SNLTerm.Direction.Input, "C")
  ce = snl.SNLScalarTerm.create(fdre, snl.SNLTerm.Direction.Input, "CE")
  r = snl.SNLScalarTerm.create(fdre, snl.SNLTerm.Direction.Input, "R")
  d = snl.SNLScalarTerm.create(fdre, snl.SNLTerm.Direction.Input, "D")
  snl.SNLParameter.create(fdre, "INIT", "0")
  
def constructFDSE(lib):
  fdse = snl.SNLDesign.createPrimitive(lib, "FDSE")
  q = snl.SNLScalarTerm.create(fdse, snl.SNLTerm.Direction.Output, "Q")
  c = snl.SNLScalarTerm.create(fdse, snl.SNLTerm.Direction.Input, "C")
  ce = snl.SNLScalarTerm.create(fdse, snl.SNLTerm.Direction.Input, "CE")
  s = snl.SNLScalarTerm.create(fdse, snl.SNLTerm.Direction.Input, "S")
  d = snl.SNLScalarTerm.create(fdse, snl.SNLTerm.Direction.Input, "D")
  snl.SNLParameter.create(fdse, "INIT", "0")

def constructRAMB36E1(lib):
  ramb36e1 = snl.SNLDesign.createPrimitive(lib, "RAMB36E1")
  addrardaddr = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, 15, 0, "ADDRARDADDR")
  addrbwraddr = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, 15, 0, "ADDRBWRADDR")
  cascadeina = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "CASCADEINA")
  cascadeinb = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "CASCADEINB")
  cascadeouta = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, "CASCADEOUTA")
  cascadeoutb = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, "CASCADEOUTB")
  clkardclk = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "CLKARDCLK")
  clkbwrclk = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "CLKBWRCLK")
  dbiterr = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, "DBITERR")
  diadi = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, 31, 0, "DIADI")
  dibdi = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, 31, 0, "DIBDI")
  dipadip = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, 3, 0, "DIPADIP")
  dipbdip = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, 3, 0, "DIPBDIP")
  doado = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, 31, 0, "DOADO")
  dobdo = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, 31, 0, "DOBDO")
  dopadop = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, 3, 0, "DOPADOP")
  dopbdop = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, 3, 0, "DOPBDOP")
  eccparity = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, 7, 0, "ECCPARITY")
  enarden = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "ENARDEN")
  enbwren = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "ENBWREN")
  injectdbiterr = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "INJECTDBITERR")
  injectsbiterr = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "INJECTSBITERR")
  rdaddrecc = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, 8, 0, "RDADDRECC")
  regcearegce = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "REGCEAREGCE")
  regceb = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "REGCEB")
  rstramarstram = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "RSTRAMARSTRAM")
  rstramb = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "RSTRAMB")
  rstregarstreg	= snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "RSTREGARSTREG")
  rstregb = snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Input, "RSTREGB")
  sbiterr	= snl.SNLScalarTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, "SBITERR")
  wea = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, 3, 0, "WEA")
  webwe = snl.SNLBusTerm.create(ramb36e1, snl.SNLTerm.Direction.Output, 7, 0, "WEBWE")
  snl.SNLParameter.create(ramb36e1, "DOA_REG", "")
  snl.SNLParameter.create(ramb36e1, "DOB_REG", "")
  snl.SNLParameter.create(ramb36e1, "INIT_A", "")
  snl.SNLParameter.create(ramb36e1, "INIT_B", "")
  for i in range(128):
    paramName = "INIT_" + hex(i)[2:].zfill(2).upper()
    snl.SNLParameter.create(ramb36e1, paramName, "")
  for i in range(16):
    paramName = "INITP_" + hex(i)[2:].zfill(2).upper()
    snl.SNLParameter.create(ramb36e1, paramName, "")
  snl.SNLParameter.create(ramb36e1, "RAM_MODE", "")
  snl.SNLParameter.create(ramb36e1, "READ_WIDTH_A", "")
  snl.SNLParameter.create(ramb36e1, "READ_WIDTH_B", "")
  snl.SNLParameter.create(ramb36e1, "WRITE_WIDTH_A", "")
  snl.SNLParameter.create(ramb36e1, "WRITE_WIDTH_B", "")
  snl.SNLParameter.create(ramb36e1, "SRVAL_A", "")
  snl.SNLParameter.create(ramb36e1, "SRVAL_B", "")
  snl.SNLParameter.create(ramb36e1, "WRITE_MODE_A", "")
  snl.SNLParameter.create(ramb36e1, "WRITE_MODE_B", "")

def constructPrimitives(lib):
  constructIBUF(lib)
  constructOBUF(lib)
  constructBUFG(lib)
  constructINV(lib)
  constructCARRY4(lib)
  constructLUT1(lib)
  constructLUT2(lib)
  constructLUT3(lib)
  constructLUT4(lib)
  constructLUT5(lib)
  constructLUT6(lib)
  constructMUXF7(lib)
  constructMUXF8(lib)
  constructSRL16E(lib)
  constructFDRE(lib)
  constructFDSE(lib)
  constructRAMB36E1(lib)
