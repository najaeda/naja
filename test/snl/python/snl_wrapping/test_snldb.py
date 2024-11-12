# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import snl
import faulthandler 

class SNLDBTest(unittest.TestCase):
  def setUp(self):
    snl.SNLUniverse.create()

  def tearDown(self):
    if snl.SNLUniverse.get():
      snl.SNLUniverse.get().destroy()

  def test(self):
    u = snl.SNLUniverse.get()
    self.assertIsNotNone(u)
    db = snl.SNLDB.create(u) 
    self.assertIsNotNone(db)
    db.dumpSNL("./test_snl")
    with self.assertRaises(SystemError) as context: db.dumpSNL(u)
    with self.assertRaises(SystemError) as context: db.dumpSNL()
    del db    
  
  def testVerilog(self):
    u = snl.SNLUniverse.get()
    self.assertIsNotNone(u)
    designs = ["../../../../../test/snl/formats/verilog/benchmarks/test0.v"]
    primitives = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
    db = snl.SNLDB.loadVerilog(primitives, designs)
    db.dumpVerilog("./test_verilog")
    with self.assertRaises(SystemError) as context: db.dumpVerilog()
    with self.assertRaises(SystemError) as context: db.dumpVerilog(-1)
    del db  

  def testLoad(self):
    u = snl.SNLUniverse.get()
    self.assertIsNotNone(u)
    db1 = snl.SNLDB.loadSNL("./test_snl")
    self.assertIsNotNone(db1)
    del db1

  def testDestroy(self):
    u = snl.SNLUniverse.get()
    self.assertIsNotNone(u)
    db = snl.SNLDB.create(u) 
    self.assertIsNotNone(db)
    db.destroy()

  def testCreationError(self):
    u = snl.SNLUniverse.get()
    db = snl.SNLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.create()
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.create("ERROR")
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL(u)
    u.destroy()
    primitives = [1]
    designs = [2]
    primitivesCorrect = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.create(u)
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL()
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL("./error")
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadVerilog("Error", "Error", "Error")
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadVerilog(primitivesCorrect, "Error")
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadVerilog("Error", "Error")
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadVerilog(primitives, designs)
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadVerilog(primitivesCorrect, designs)
    
if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()