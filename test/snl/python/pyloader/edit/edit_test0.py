# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
import snl

def edit():
  logging.basicConfig(filename='edit_test0.log', filemode='w' ,level=logging.DEBUG)
  universe = snl.SNLUniverse.get()
  top = universe.getTopDesign()
  if top is None:
    logging.error('SNLUniverse does not contain any top SNLDesign')
  else:
    logging.info('Found top design ' + str(top))

  #change the name of instance0 to instance1
  instance0 = top.getInstance('instance0')
  if instance0 is not None:
    instance0.setName('instance1')