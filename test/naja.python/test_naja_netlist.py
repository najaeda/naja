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
    pass

  def tearDown(self):
    pass

  def testLoader(self):
    desingFiles = ["../../../test/snl/formats/verilog/benchmarks/test0.v"]
    primitives = ["../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
    loader = netlist.Loader()
    loader.init()
    loader.loadLibertyPrimitives(primitives)
    loader.loadVerilog(desingFiles)
    loader.verify()
    db = loader.getDB()
    del db

if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()
