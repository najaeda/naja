# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
import snl

def naja_edit(option1=True, option2=False):
  logging.basicConfig(filename='naja_edit_test_args0.log', filemode='w' ,level=logging.DEBUG)
  logging.info('option1 value is: ' + str(option1))
  logging.info('option2 value is: ' + str(option2))