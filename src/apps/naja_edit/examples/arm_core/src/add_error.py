import logging
from naja import snl

def edit():
  logging.basicConfig(filename='edit.log', filemode='w' ,level=logging.DEBUG)
  universe = snl.SNLUniverse.get()
  if universe is None:
    logging.critical('No loaded SNLUniverse')
    return 1
  top = universe.getTopDesign()
  if top is None:
    logging.critical('SNLUniverse does not contain any top SNLDesign')
    return 1
  else:
    logging.info('Found top design ' + str(top))

  #get n instances and destroy
  n = 20
  destroyed = 0
  for instance in top.getInstances():
    destroyed += 1
    instance.destroy()
    if destroyed > n:
      break

  #get n nets and destroy
  n = 20
  destroyed = 0
  for net in top.getNets():
    destroyed += 1
    net.destroy()
    if destroyed > n:
      break


  #lut14Mask = lut14.getInstParameter('INIT')
  #if lut14Mask is None:
  #  logging.critical('cannot find \'INIT\' in ' + str(lut14))
  #  return 1
  #else:
  #  logging.info('Found ' + str(lut14Mask))

  #lut14Mask.setValue('8\'hfb')



