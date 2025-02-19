# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
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
    snl.SNLUniverse.get().setTopDB(db)
    self.assertEqual(snl.SNLUniverse.get().getTopDB(), db)
    self.assertEqual(db.getID(), 1) # ID starts from 1, 0 is reserved for DB0
    with self.assertRaises(RuntimeError) as context: snl.SNLUniverse.get().setTopDB("Error")
    del db    
  
  def testVerilog(self):
    u = snl.SNLUniverse.get()
    db = snl.SNLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "test0.v")]
    primitives = [os.path.join(formats_path, "liberty", "benchmarks", "asap7_excerpt", "test0.lib")]
    db.loadLibertyPrimitives(primitives)
    db.loadVerilog(verilogs)
    db.dumpVerilog("./test_verilog")
    with self.assertRaises(RuntimeError) as context: db.dumpVerilog()
    with self.assertRaises(RuntimeError) as context: db.dumpVerilog(-1)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives("./error.lib")
  
  def testVerilogNoAssigns(self):
    u = snl.SNLUniverse.get()
    db = snl.SNLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "test0.v")]
    primitives = [os.path.join(formats_path, "liberty", "benchmarks", "asap7_excerpt", "test0.lib")]
    db.loadLibertyPrimitives(primitives)
    db.loadVerilog(verilogs, keep_assigns=False)
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(verilogs, keep_assign=False)

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
    primitivesNoExtension = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0"]
    primitivesCorrect = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
    primitivesWrongExtension = ["../../../../../test/snl/formats/liberty/benchmarks/asap7_excerpt/test0.sd"]
    with self.assertRaises(RuntimeError) as context: db.loadVerilog("Error", "Error")
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives("Error", "Error")
    with self.assertRaises(RuntimeError) as context: db.loadVerilog("Error")
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives("Error")
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitives)
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(designs)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitivesNoExtension)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitivesWrongExtension)
    u.destroy()
    with self.assertRaises(RuntimeError) as context: snl.SNLDB.create(u)
    #with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL()
    #with self.assertRaises(RuntimeError) as context: snl.SNLDB.loadSNL("./error")
    
if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()