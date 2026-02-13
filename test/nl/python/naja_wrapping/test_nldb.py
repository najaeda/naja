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

  def testVerilogLoadingOptions(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)

    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "conflicting_name_designs.v")]
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(verilogs, conflicting_design_name_policy="forbid")

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadVerilog(verilogs, conflicting_design_name_policy="first")
    self.assertIsNotNone(top)
    self.assertEqual(top.getName(), 'clash')
    self.assertEqual(1, sum(1 for t in top.getTerms()))
    self.assertEqual(1, sum(1 for t in top.getScalarTerms()))
    term = next(iter(top.getScalarTerms()))
    self.assertIsNotNone(term)
    self.assertEqual(term.getName(), 'A')

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadVerilog(verilogs, conflicting_design_name_policy="last")
    self.assertIsNotNone(top)
    self.assertEqual(top.getName(), 'clash')
    self.assertEqual(1, sum(1 for t in top.getTerms()))
    self.assertEqual(1, sum(1 for t in top.getScalarTerms()))
    term = next(iter(top.getScalarTerms()))
    self.assertIsNotNone(term)
    self.assertEqual(term.getName(), 'D') 

    #errors
    db.destroy()
    db = naja.NLDB.create(u)
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(verilogs, conflicting_design_name_policy='verify') 
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(verilogs, conflicting_design_name_policy=1)
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(verilogs, conflicting_design_name_policy='foo')


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
    db.dumpNajaIF(naja_dir)
    #destroy everything
    naja.NLUniverse.get().destroy()

    #load NajaIF
    naja.NLDB.loadNajaIF(naja_dir)
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

  def testVerilogPreprocessEnabled(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "preprocess_top.v")]

    with self.assertRaises(RuntimeError) as context: db.loadVerilog(verilogs)

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadVerilog(verilogs, preprocess_enabled=True)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())
    self.assertEqual(1, sum(1 for _ in top.getInstances()))
    inst = top.getInstance("u0")
    self.assertIsNotNone(inst)
    self.assertEqual("child", inst.getModel().getName())
    a = top.getBusTerm("a")
    self.assertIsNotNone(a)
    self.assertEqual(1, a.getMSB())
    self.assertEqual(0, a.getLSB())
    self.assertEqual(2, a.getWidth())

  def testSystemVerilog(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    sv_file = os.path.join(formats_path, "systemverilog", "benchmarks", "simple", "simple.sv")
    output_path = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(output_path)
    json_path = os.path.join(output_path, "simple_elaborated_ast_python.json")

    top = db.loadSystemVerilog([sv_file], elaborated_ast_json_path=json_path)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())
    self.assertEqual(3, sum(1 for _ in top.getTerms()))
    self.assertTrue(os.path.exists(json_path))

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadSystemVerilog([sv_file], keep_assigns=False)
    self.assertIsNotNone(top)

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
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadNajaIF(u)
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadNajaIF("./test_verilogError.v")
    primitives = [1]
    designs = [2]
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    liberty_path = os.path.join(formats_path, 'liberty')
    primitivesNoExtension = [os.path.join(liberty_path, "benchmarks/asap7_excerpt/test0")]
    primitivesCorrect = [os.path.join(liberty_path, "benchmarks/asap7_excerpt/test0.lib")]
    primitivesWrongExtension = [os.path.join(liberty_path, "benchmarks/asap7_excerpt/test0.sd")]
    systemverilog_path = os.path.join(formats_path, "systemverilog")
    svFile = os.path.join(systemverilog_path, "benchmarks/simple/simple.sv")
    with self.assertRaises(RuntimeError) as context: db.loadVerilog("Error", "Error")
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog("Error")
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog(designs)
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([1])
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([svFile], elaborated_ast_json_path=1)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives("Error", "Error")
    with self.assertRaises(RuntimeError) as context: db.loadVerilog("Error")
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives("Error")
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitives)
    with self.assertRaises(RuntimeError) as context: db.loadVerilog(designs)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitivesNoExtension)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitivesWrongExtension)
    u.destroy()
    with self.assertRaises(RuntimeError) as context: naja.NLDB.create(u)
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadNajaIF()
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadNajaIF("./error")

  def testDumpError(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: db.dumpNajaIF()
    with self.assertRaises(RuntimeError) as context: db.dumpNajaIF(db)

if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()
