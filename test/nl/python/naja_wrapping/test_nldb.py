# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import naja
import faulthandler 

class SNLDBTest(unittest.TestCase):
  def setUp(self):
    naja.NLUniverse.create()

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def test(self):
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(db)
    naja.NLUniverse.get().setTopDB(db)
    self.assertEqual(naja.NLUniverse.get().getTopDB(), db)
    self.assertEqual(db.getID(), 1) # ID starts from 1, 0 is reserved for DB0
    with self.assertRaises(RuntimeError) as context: naja.NLUniverse.get().setTopDB("Error")
    del db    
  
  def testVerilog(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
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
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
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

    naja_path = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(naja_path)
    naja_dir = os.path.join(naja_path, "test.naja")
    db.dumpSNL(naja_dir)
    #destroy everything
    naja.NLUniverse.get().destroy()

    #load SNL
    naja.NLDB.loadSNL(naja_dir)
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = u.getTopDB()
    self.assertIsNotNone(db)
    self.assertEqual(db.getID(), 1)

  
  def testVerilogNoAssigns(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "test0.v")]
    primitives = [os.path.join(formats_path, "liberty", "benchmarks", "asap7_excerpt", "test0.lib")]
    db.loadLibertyPrimitives(primitives)
    db.loadVerilog(verilogs, keep_assigns=False)
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(verilogs, keep_assign=False)

  def testDestroy(self):
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(db)
    db.destroy()

  def testCreationError(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: naja.NLDB.create()
    with self.assertRaises(RuntimeError) as context: naja.NLDB.create("ERROR")
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadSNL(u)
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadSNL("./test_verilogError.v")
    primitives = [1]
    designs = [2]
    primitivesNoExtension = ["../../../../../test/naja/formats/liberty/benchmarks/asap7_excerpt/test0"]
    primitivesCorrect = ["../../../../../test/naja/formats/liberty/benchmarks/asap7_excerpt/test0.lib"]
    primitivesWrongExtension = ["../../../../../test/naja/formats/liberty/benchmarks/asap7_excerpt/test0.sd"]
    with self.assertRaises(RuntimeError) as context: db.loadVerilog("Error", "Error")
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives("Error", "Error")
    with self.assertRaises(RuntimeError) as context: db.loadVerilog("Error")
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives("Error")
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitives)
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(designs)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitivesNoExtension)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitivesWrongExtension)
    u.destroy()
    with self.assertRaises(RuntimeError) as context: naja.NLDB.create(u)
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadSNL()
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadSNL("./error")

  def testDumpError(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: db.dumpSNL()
    with self.assertRaises(RuntimeError) as context: db.dumpSNL(db)
    
if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()
