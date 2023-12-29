import logging
import snl

def edit():
  logging.basicConfig(filename='edit.log', filemode='w' ,level=logging.DEBUG)
  universe = snl.SNLUniverse.get()
  top = universe.getTopDesign()
  if top is None:
    logging.error('SNLUniverse does not contain any top SNLDesign')
  else:
    logging.info('Found top design ' + str(top))

  #change the name of net 'r' to 'RR'
  rNet = top.getNet('r')
  if rNet is not None:
    rNet.setName('RR')

  #destroy net 'sum'
  sumNet = top.getNet('sum')
  if sumNet is not None:
    sumNet.destroy()


  prims = None
  db = top.getDB()
  for lib in db.getLibraries():
    if lib.isPrimitives():
      prims = lib
      logging.info('Found primitives library ' + str(lib))

  #create a top net 'MyNet'
  topNet = snl.SNLScalarNet.create(top, 'MyNet')

  if prims is not None:
    lut3 = prims.getDesign('LUT3')

    #create a top instance of 'LUT3' named 'NewInstance'
    lut3Instance = snl.SNLInstance.create(top, lut3, 'NewInstance')
    #connect 'I0' to 'MyNet'
    lut3I0 = lut3Instance.getInstTerm(lut3.getScalarTerm('I0'))
    if lut3I0 is not None:
      lut3I0.setNet(topNet)
