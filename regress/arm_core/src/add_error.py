import logging
import snl

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

  #find lut in top
  lut14 = top.getInstance("_014_")
  lut15 = top.getInstance("_015_")
  lut16 = top.getInstance("_016_")
  lut17 = top.getInstance("_017_")
  if lut14 is not None:
    logging.info('Destroy ' + str(lut14))
    lut14.destroy()
  if lut15 is not None:
    logging.info('Destroy ' + str(lut15))
    lut15.destroy()
  if lut16 is not None:
    logging.info('Destroy ' + str(lut16))
    lut16.destroy()
  if lut17 is not None:
    logging.info('Destroy ' + str(lut17))
    lut17.destroy()

  #lut14Mask = lut14.getInstParameter('INIT')
  #if lut14Mask is None:
  #  logging.critical('cannot find \'INIT\' in ' + str(lut14))
  #  return 1
  #else:
  #  logging.info('Found ' + str(lut14Mask))

  #lut14Mask.setValue('8\'hfb')



