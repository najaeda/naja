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
    del db    
  
  def testVerilog(self):
    u = snl.SNLUniverse.get()
    db = snl.SNLDB.create(u) 
    self.assertIsNotNone(u)
    designs = ["../../../../../test/snl/formats/verilog/benchmarks/test0.v"]
    primitives = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
    db.loadLibertyPrimitives(primitives)
    db.loadVerilog(designs)
    db.dumpVerilog("./test_verilog")
    with self.assertRaises(SystemError) as context: db.dumpVerilog()
    with self.assertRaises(SystemError) as context: db.dumpVerilog(-1)
    del db  

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
    #with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL(u)
    #with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL("./test_verilogError.v")
    primitives = [1]
    designs = [2]
    primitivesNoExtention = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0"]
    primitivesCorrect = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
    primitivesWrongExtention = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.sd"]
    with self.assertRaises(SystemError) as context: db.loadVerilog("Error", "Error")
    with self.assertRaises(SystemError) as context: db.loadLibertyPrimitives("Error", "Error")
    with self.assertRaises(SystemError) as context: db.loadVerilog("Error")
    with self.assertRaises(SystemError) as context: db.loadLibertyPrimitives("Error")
    with self.assertRaises(SystemError) as context: db.loadLibertyPrimitives(primitives)
    with self.assertRaises(SystemError) as context: db.loadVerilog(designs)
    with self.assertRaises(SystemError) as context: db.loadLibertyPrimitives(primitivesNoExtention)
    with self.assertRaises(SystemError) as context: db.loadLibertyPrimitives(primitivesWrongExtention)
    u.destroy()
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.create(u)
    #with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL()
    #with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL("./error")
    
if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()