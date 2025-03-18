# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import snl
import faulthandler 

class SNLDBTest(unittest.TestCase):
  def setUp(self):
    snl.NLUniverse.create()

  def tearDown(self):
    if snl.NLUniverse.get():
      snl.NLUniverse.get().destroy()

  def test(self):
    u = snl.NLUniverse.get()
    self.assertIsNotNone(u)
    db = snl.NLDB.create(u) 
    self.assertIsNotNone(db)
    snl.NLUniverse.get().setTopDB(db)
    self.assertEqual(snl.NLUniverse.get().getTopDB(), db)
    self.assertEqual(db.getID(), 1) # ID starts from 1, 0 is reserved for DB0
    with self.assertRaises(RuntimeError) as context: snl.NLUniverse.get().setTopDB("Error")
    del db    
  
  def testVerilog(self):
    u = snl.NLUniverse.get()
    db = snl.NLDB.create(u) 
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

  def testSNLFormat(self):
    u = snl.NLUniverse.get()
    db = snl.NLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "test0.v")]
    primitives = [os.path.join(formats_path, "liberty", "benchmarks", "asap7_excerpt", "test0.lib")]
    db.loadLibertyPrimitives(primitives)
    db.loadVerilog(verilogs)
    self.assertIsNotNone(db.getTopDesign())
    top = db.getTopDesign()
    self.assertTrue(top.isTopDesign())
    self.assertEqual(top.getDB(), db)
    self.assertTrue(db.isTopDB())

    snl_path = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(snl_path)
    snl_dir = os.path.join(snl_path, "test.snl")
    db.dumpSNL(snl_dir)
    #destroy everything
    snl.NLUniverse.get().destroy()

    #load SNL
    snl.NLDB.loadSNL(snl_dir)
    u = snl.NLUniverse.get()
    self.assertIsNotNone(u)
    db = u.getTopDB()
    self.assertIsNotNone(db)
    self.assertEqual(db.getID(), 1)

  
  def testVerilogNoAssigns(self):
    u = snl.NLUniverse.get()
    db = snl.NLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "test0.v")]
    primitives = [os.path.join(formats_path, "liberty", "benchmarks", "asap7_excerpt", "test0.lib")]
    db.loadLibertyPrimitives(primitives)
    db.loadVerilog(verilogs, keep_assigns=False)
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(verilogs, keep_assign=False)

  def testDestroy(self):
    u = snl.NLUniverse.get()
    self.assertIsNotNone(u)
    db = snl.NLDB.create(u) 
    self.assertIsNotNone(db)
    db.destroy()

  def testCreationError(self):
    u = snl.NLUniverse.get()
    db = snl.NLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: snl.NLDB.create()
    with self.assertRaises(RuntimeError) as context: snl.NLDB.create("ERROR")
    with self.assertRaises(RuntimeError) as context: snl.NLDB.loadSNL(u)
    with self.assertRaises(RuntimeError) as context: snl.NLDB.loadSNL("./test_verilogError.v")
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
    with self.assertRaises(RuntimeError) as context: snl.NLDB.create(u)
    with self.assertRaises(RuntimeError) as context: snl.NLDB.loadSNL()
    with self.assertRaises(RuntimeError) as context: snl.NLDB.loadSNL("./error")

  def testDumpError(self):
    u = snl.NLUniverse.get()
    db = snl.NLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: db.dumpSNL()
    with self.assertRaises(RuntimeError) as context: db.dumpSNL(db)
    
if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()