# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
import naja

def edit():
  logging.basicConfig(filename='edit_test0.log', filemode='w' ,level=logging.DEBUG)
  universe = naja.NLUniverse.get()
  top = universe.getTopDesign()
  if top is None:
    logging.error('NLUniverse does not contain any top SNLDesign')
  else:
    logging.info('Found top design ' + str(top))

  #change the name of instance0 to instance00
  instance0 = top.getInstance('instance0')
  if instance0 is not None:
    instance0.setName('instance00')
  bbox_instance = top.getInstance('instance1')
  if bbox_instance.getModel().isBlackBox():
    bbox_instance.setName('bbox_instance')
  assign_instance = top.getInstance('instance2')
  if assign_instance.getModel().isAssign():
    assign_instance.setName('assign_instance')
