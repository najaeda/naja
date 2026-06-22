# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import gzip
import shutil
import tempfile
import unittest
import zipfile
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

  def testLoadLibertyPrimitivesRenamedFiles(self):
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    source = os.path.join(
      formats_path,
      "liberty",
      "benchmarks",
      "tests",
      "small.lib")
    self.assertTrue(os.path.exists(source))

    with tempfile.TemporaryDirectory() as tempdir:
      lib_suffix_path = os.path.join(tempdir, "small.lib_tt")
      gzip_path = os.path.join(tempdir, "small.memory")
      zip_path = os.path.join(tempdir, "small.bundle")

      shutil.copyfile(source, lib_suffix_path)
      with open(source, "rb") as input_file:
        with gzip.open(gzip_path, "wb") as gzip_file:
          shutil.copyfileobj(input_file, gzip_file)
      with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.write(source, "memories/small.lib_tt")

      for path in (lib_suffix_path, gzip_path, zip_path):
        with self.subTest(path=path):
          db = naja.NLDB.create(naja.NLUniverse.get())
          db.loadLibertyPrimitives([path])
          primitives = db.getLibrary("small_lib")
          self.assertIsNotNone(primitives)
          self.assertIsNotNone(primitives.getSNLDesign("and2"))
          db.destroy()

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

  def testAssignInstancePartition(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.NamedTemporaryFile("w", suffix=".v", delete=False) as source:
      source.write("module child(input i, output o); assign o = i; endmodule\n")
      source.write(
        "module top(input i, output o); "
        "wire n; child u_child(.i(i), .o(n)); assign o = n; endmodule\n")
      source_path = source.name
    try:
      top = db.loadVerilog([source_path])
      instances = list(top.getInstances())
      non_assign_instances = list(top.getNonAssignInstances())
      assign_instances = list(top.getAssignInstances())

      self.assertEqual(
        instances,
        non_assign_instances + assign_instances)
      self.assertTrue(all(not inst.getModel().isAssign() for inst in non_assign_instances))
      self.assertTrue(all(inst.getModel().isAssign() for inst in assign_instances))
      self.assertEqual(1, len(non_assign_instances))
      self.assertEqual(1, len(assign_instances))
    finally:
      os.remove(source_path)

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
    diagnostics_path = os.path.join(output_path, "simple_diagnostics_python.txt")

    top = db.loadSystemVerilog(
      [sv_file],
      elaborated_ast_json_path=json_path,
      diagnostics_report_path=diagnostics_path)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())
    self.assertEqual(3, sum(1 for _ in top.getTerms()))
    self.assertTrue(top.hasSourceLoc())
    source_loc = top.getSourceLoc()
    self.assertIsInstance(source_loc, tuple)
    self.assertEqual(5, len(source_loc))
    self.assertTrue(source_loc[0].endswith("systemverilog/benchmarks/simple/simple.sv"))
    self.assertGreaterEqual(source_loc[1], 1)
    self.assertGreaterEqual(source_loc[2], 1)
    self.assertGreaterEqual(source_loc[3], source_loc[1])
    self.assertGreaterEqual(source_loc[4], 1)
    term = top.getTerm("a")
    self.assertIsNotNone(term)
    self.assertTrue(term.hasSourceLoc())
    self.assertTrue(term.getSourceLoc()[0].endswith("systemverilog/benchmarks/simple/simple.sv"))
    net = top.getNet("y")
    self.assertIsNotNone(net)
    self.assertTrue(net.hasSourceLoc())
    self.assertTrue(net.getSourceLoc()[0].endswith("systemverilog/benchmarks/simple/simple.sv"))
    self.assertTrue(os.path.exists(json_path))
    self.assertTrue(os.path.exists(diagnostics_path))

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadSystemVerilog([sv_file], keep_assigns=False)
    self.assertIsNotNone(top)

    db.destroy()
    db = naja.NLDB.create(u)
    flist_path = os.path.join(output_path, "simple_systemverilog_python.f")
    with open(flist_path, "w", encoding="utf-8") as flist:
      flist.write(f"{sv_file}\n")
    top = db.loadSystemVerilog([], flist=flist_path)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadSystemVerilog([sv_file], suppress_warnings=["width-trunc"])
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())

    db.destroy()
    db = naja.NLDB.create(u)
    with tempfile.NamedTemporaryFile("w", suffix=".sv", delete=False) as defineFile:
      defineFile.write("`ifdef SYNTHESIS\n")
      defineFile.write("module synth_top(input logic a, output logic y);\n")
      defineFile.write("  assign y = a;\n")
      defineFile.write("endmodule\n")
      defineFile.write("`else\n")
      defineFile.write("module sim_top(input logic a, output logic y);\n")
      defineFile.write("  assign y = ~a;\n")
      defineFile.write("endmodule\n")
      defineFile.write("`endif\n")
      definePath = defineFile.name
    try:
      top = db.loadSystemVerilog([definePath], defines=["SYNTHESIS"])
      self.assertIsNotNone(top)
      self.assertEqual("synth_top", top.getName())
    finally:
      if os.path.exists(definePath):
        os.remove(definePath)

  def testDesignDumpVerilogOptions(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    sv_file = os.path.join(
      formats_path, "systemverilog", "benchmarks", "simple", "simple.sv")

    top = db.loadSystemVerilog([sv_file])
    self.assertIsNotNone(top)

    with tempfile.TemporaryDirectory() as dump_dir:
      without_rtl_infos = os.path.join(dump_dir, "simple_default_no_rtl_infos.v")
      top.dumpVerilog(
        path=dump_dir,
        top_file_name=os.path.basename(without_rtl_infos))
      with open(without_rtl_infos, "r", encoding="utf-8") as dumped_file:
        dumped_text = dumped_file.read()
      self.assertNotIn("sv_src_file", dumped_text)

      with_rtl_infos = os.path.join(dump_dir, "simple_with_rtl_infos.v")
      top.dumpVerilog(
        path=dump_dir,
        top_file_name=os.path.basename(with_rtl_infos),
        dumpRTLInfosAsAttributes=True)
      with open(with_rtl_infos, "r", encoding="utf-8") as dumped_file:
        dumped_text = dumped_file.read()
      self.assertNotIn("sv_src_file", dumped_text)
      self.assertIn('naja_sv_src="', dumped_text)

      with_verbose_rtl_infos = os.path.join(dump_dir, "simple_with_verbose_rtl_infos.v")
      top.dumpVerilog(
        path=dump_dir,
        top_file_name=os.path.basename(with_verbose_rtl_infos),
        dumpRTLInfosAsAttributes=True,
        rtlInfoDumpMode="VerboseAttributes")
      with open(with_verbose_rtl_infos, "r", encoding="utf-8") as dumped_file:
        dumped_text = dumped_file.read()
      self.assertIn("sv_src_file", dumped_text)
      self.assertNotIn('naja_sv_src="', dumped_text)

      none_rtl_infos = os.path.join(dump_dir, "simple_none_rtl_infos.v")
      top.dumpVerilog(
        path=dump_dir,
        top_file_name=os.path.basename(none_rtl_infos),
        dumpRTLInfosAsAttributes=True,
        rtlInfoDumpMode="None")
      with open(none_rtl_infos, "r", encoding="utf-8") as dumped_file:
        dumped_text = dumped_file.read()
      self.assertNotIn("sv_src_file", dumped_text)
      self.assertNotIn('naja_sv_src="', dumped_text)

      with self.assertRaises(RuntimeError) as context:
        top.dumpVerilog(
          path=dump_dir,
          top_file_name="simple_invalid_rtl_infos.v",
          dumpRTLInfosAsAttributes=True,
          rtlInfoDumpMode="Invalid")
      self.assertIn("invalid rtlInfoDumpMode", str(context.exception))

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
    missingSvFile = os.path.join(systemverilog_path, "benchmarks/missing/missing.sv")
    with tempfile.NamedTemporaryFile("w", suffix=".sv", delete=False) as multiTopFile:
      multiTopFile.write("module top_a; endmodule\n")
      multiTopFile.write("module top_b; endmodule\n")
      multiTopPath = multiTopFile.name
    with self.assertRaises(RuntimeError) as context: db.loadVerilog("Error", "Error")
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog()
    with self.assertRaises(RuntimeError) as context:
      db.loadSystemVerilog([missingSvFile])
    self.assertIn("Error while parsing SystemVerilog:", str(context.exception))
    try:
      with self.assertRaises(RuntimeError) as context:
        db.loadSystemVerilog([multiTopPath])
      self.assertIn("No top design was found after parsing systemverilog", str(context.exception))
    finally:
      if os.path.exists(multiTopPath):
        os.remove(multiTopPath)
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog("Error")
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog(designs)
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([1])
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([svFile], elaborated_ast_json_path=1)
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([svFile], diagnostics_report_path=1)
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([svFile], defines=1)
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([svFile], defines=[1])
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([svFile], suppress_warnings=1)
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([svFile], suppress_warnings=[1])
    with self.assertRaises(RuntimeError) as context: db.loadSystemVerilog([svFile], flist=1)
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
