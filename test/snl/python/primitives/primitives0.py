import snl

def constructLUT2(lib):
  lut2 = snl.SNLDesign.create(lib) #, "LUT2")

def constructPrimitives(lib):
  f = open('debug.txt', 'w')
  f.write("DEBUG\n")
  f.write("DEBUG\n")
  f.write("DEBUG\n")
  f.write("DEBUG\n")
  f.write("DEBUG\n")
  if (lib is None):
    f.write("lib is None")
  else:
    f.write("lib is not None")
  f.write(lib)
  f.close()

  #raise Exception(str(lib))
  constructLUT2(lib)