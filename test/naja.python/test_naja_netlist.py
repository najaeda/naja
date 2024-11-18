# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os

# Get the PYTHONPATH environment variable
pythonpath = os.environ.get('PYTHONPATH')

if pythonpath:
    # os.pathsep is the separator used in PYTHONPATH (':' on Unix, ';' on Windows)
    paths = pythonpath.split(os.pathsep)
    print("PYTHONPATH contains the following directories:")
    for path in paths:
        print(path)
else:
    print("PYTHONPATH is not set.")

import unittest
import faulthandler 
from naja import netlist

class NajaNetlistTest(unittest.TestCase):
  def setUp(self):
    self.loader = netlist.Loader()
    self.loader.init()

  def tearDown(self):
    pass
    #if snl.SNLUniverse.get():
    #  snl.SNLUniverse.get().destroy()

  def testLoader(self):
    self.assertIsNotNone(self.loader)
    self.assertIsNotNone(self.loader.getDB())

if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()
