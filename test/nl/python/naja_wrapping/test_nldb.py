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

INTENT_MINI_SV = """\
package mini_pkg;
  typedef logic [7:0] byte_t;
  typedef enum logic [1:0] {
    ST_IDLE = 2'b00,
    ST_RUN  = 2'b01,
    ST_DONE = 2'b11
  } state_e;
  typedef struct packed {
    logic   valid;
    byte_t  data;
    state_e state;
  } payload_t;
  typedef union packed {
    logic [7:0] raw;
    byte_t      data;
  } overlay_t;
  localparam int PLEN = (32 == 32) ? 34 : 56;
endpackage

module intent_mini #(
    parameter int DEPTH = 4
) (
    input  logic clk,
    input  logic rst_n
);
  import mini_pkg::*;
  localparam int IDX_W = $clog2(DEPTH);

  mini_pkg::state_e state_q;
  byte_t             byte_q;
  logic [3:0]        plain_q;
  payload_t          payload_q;
  overlay_t          overlay_q;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) state_q <= ST_IDLE;
    else        state_q <= ST_RUN;
  end
endmodule
"""

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
    manifest = naja.snapshot_manifest(naja_dir)
    self.assertEqual(manifest["schema_version"], (0, 1, 0))
    self.assertEqual(manifest["producer_version"], naja.getVersion())
    self.assertEqual(manifest["producer_git_hash"], naja.getGitHash())
    #destroy everything
    naja.NLUniverse.get().destroy()

    manifest = naja.snapshot_manifest(naja_dir)
    self.assertEqual(manifest["schema_version"], (0, 1, 0))
    self.assertIsNone(naja.NLUniverse.get())

    #load NajaIF
    naja.NLDB.loadNajaIF(naja_dir)
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = u.getTopDB()
    self.assertIsNotNone(db)
    self.assertEqual(db.getID(), 1)

    with open(os.path.join(naja_dir, "snl.mf"), "w", encoding="utf-8") as manifest_file:
      manifest_file.write("V 0 1 0\n")
      manifest_file.write("P test-producer test-hash\n")
    naja.NLUniverse.get().destroy()
    with self.assertRaises(RuntimeError) as context:
      naja.NLDB.loadNajaIF(naja_dir)
    self.assertIn("Incompatible SNL snapshot producer", str(context.exception))

  def testSnapshotManifestInvalidArguments(self):
    with self.assertRaisesRegex(
        RuntimeError, "malformed naja snapshot_manifest"):
      naja.snapshot_manifest()

    with self.assertRaisesRegex(
        RuntimeError, "snapshot_manifest argument should be a file path"):
      naja.snapshot_manifest(42)
  
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
    self.assertIsNone(naja.live_compilation())
    self.assertIsNone(naja.ast_symbol_of(top))
    with self.assertRaises(RuntimeError):
      naja.ast_symbol_of(db)
    with self.assertRaises(ValueError):
      naja.snl_objects_of(object())

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadSystemVerilog([sv_file], keep_ast_link=True)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())
    self.assertIsNotNone(naja.live_compilation())
    top_symbol = naja.ast_symbol_of(top)
    self.assertIsNotNone(top_symbol)
    self.assertIn(top, naja.snl_objects_of(top_symbol))
    net_symbol = naja.ast_symbol_of(top.getNet("y"))
    self.assertIsNotNone(net_symbol)
    self.assertIn(top.getNet("y"), naja.snl_objects_of(net_symbol))
    term_symbol = naja.ast_symbol_of(top.getTerm("a"))
    self.assertIsNotNone(term_symbol)
    self.assertIn(top.getTerm("a"), naja.snl_objects_of(term_symbol))

    # A correctly named capsule whose pointer is unknown to the live registry
    # is valid input and produces an empty result.
    import ctypes
    capsule_new = ctypes.pythonapi.PyCapsule_New
    capsule_new.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]
    capsule_new.restype = ctypes.py_object
    capsule_name = b"naja.frontend.Symbol"
    unknown_symbol = capsule_new(ctypes.c_void_p(1), capsule_name, None)
    self.assertEqual([], naja.snl_objects_of(unknown_symbol))

    db.destroy()
    self.assertIsNone(naja.live_compilation())
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

  def testSystemVerilogTypedExceptions(self):
    self.assertTrue(issubclass(naja.SystemVerilogError, RuntimeError))
    self.assertTrue(issubclass(
      naja.SystemVerilogSyntaxError, naja.SystemVerilogError))
    self.assertTrue(issubclass(
      naja.SystemVerilogUnsupportedError, naja.SystemVerilogError))
    self.assertTrue(issubclass(
      naja.SystemVerilogInternalError, naja.SystemVerilogError))

    u = naja.NLUniverse.get()
    with tempfile.TemporaryDirectory() as tempdir:
      syntax_path = os.path.join(tempdir, "typed_syntax_error.sv")
      with open(syntax_path, "w", encoding="utf-8") as source:
        source.write(
          "module typed_syntax_error(input logic a, output logic y);\n"
          "  assign y = a\n"
          "endmodule\n")

      db = naja.NLDB.create(u)
      with self.assertRaises(naja.SystemVerilogSyntaxError) as context:
        db.loadSystemVerilog([syntax_path])
      syntax_error = context.exception
      self.assertIsInstance(syntax_error, RuntimeError)
      self.assertIn("Error while parsing SystemVerilog: ", str(syntax_error))
      self.assertTrue(syntax_error.diagnostics)
      diagnostic = syntax_error.diagnostics[0]
      self.assertEqual("error", diagnostic["severity"])
      self.assertTrue(diagnostic["file"].endswith("typed_syntax_error.sv"))
      self.assertGreater(diagnostic["line"], 0)
      self.assertGreater(diagnostic["column"], 0)
      self.assertTrue(diagnostic["message"])
      db.destroy()

      unsupported_path = os.path.join(tempdir, "typed_unsupported.sv")
      with open(unsupported_path, "w", encoding="utf-8") as source:
        source.write("""\
module typed_unsupported(input logic a, output logic y);
  task do_check(input logic value);
  endtask
  initial begin
    do_check(a);
  end
  assign y = a;
endmodule
""")

      db = naja.NLDB.create(u)
      with self.assertRaises(naja.SystemVerilogUnsupportedError) as context:
        db.loadSystemVerilog([unsupported_path])
      unsupported_error = context.exception
      self.assertIsInstance(unsupported_error, RuntimeError)
      self.assertIn("Unsupported SystemVerilog elements encountered", str(unsupported_error))
      self.assertTrue(unsupported_error.unsupported_elements)
      unsupported = unsupported_error.unsupported_elements[0]
      self.assertIn("Unsupported initial block", unsupported["message"])
      self.assertEqual(
        "sv.unsupported.procedural_block",
        unsupported["code"])
      db.destroy()

      valid_path = os.path.join(tempdir, "typed_internal.sv")
      with open(valid_path, "w", encoding="utf-8") as source:
        source.write("module typed_internal; endmodule\n")

      db = naja.NLDB.create(u)
      with self.assertRaises(naja.SystemVerilogInternalError) as context:
        db.loadSystemVerilog([valid_path], diagnostics_report_path="")
      self.assertIsInstance(context.exception, RuntimeError)
      self.assertIn("Empty path for diagnostics report dump", str(context.exception))

  def testSystemVerilogDiagnosticsReportCanBeDisabled(self):
    u = naja.NLUniverse.get()
    with tempfile.TemporaryDirectory() as tempdir:
      sv_path = os.path.join(tempdir, "console_only.sv")
      with open(sv_path, "w", encoding="utf-8") as source:
        source.write("module console_only; endmodule\n")
      original_cwd = os.getcwd()
      try:
        os.chdir(tempdir)
        db = naja.NLDB.create(u)
        top = db.loadSystemVerilog([sv_path], diagnostics_report_path=None)
        self.assertIsNotNone(top)
        self.assertFalse(os.path.exists("naja_sv_diagnostics.log"))
        db.destroy()
      finally:
        os.chdir(original_cwd)

  def testSystemVerilogIntentAPI(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    self.assertFalse(naja.intent_available())
    with self.assertRaises(RuntimeError):
      naja.intent_type_of(db)
    with self.assertRaises(RuntimeError):
      naja.intent_parameters_of(db)
    with self.assertRaises(RuntimeError):
      naja.intent_package_member("mini_pkg")

    with tempfile.TemporaryDirectory() as tempdir:
      sv_file = os.path.join(tempdir, "intent_mini.sv")
      with open(sv_file, "w", encoding="utf-8") as fixture:
        fixture.write(INTENT_MINI_SV)

      top = db.loadSystemVerilog([sv_file], keep_ast_link=True)
      self.assertIsNotNone(top)
      self.assertTrue(naja.intent_available())

      state_q = top.getNet("state_q")
      self.assertIsNotNone(state_q)
      type_rec = naja.intent_type_of(state_q)
      self.assertEqual("mini_pkg::state_e", type_rec["type"])
      self.assertEqual("enum", type_rec["canonical_kind"])
      self.assertTrue(type_rec["src"].endswith("intent_mini.sv:29"))
      enum = type_rec["enum"]
      self.assertEqual(2, enum["width"])
      self.assertTrue(enum["decl"].endswith("intent_mini.sv:3"))
      self.assertEqual(
        {"ST_IDLE": "2'b00", "ST_RUN": "2'b01", "ST_DONE": "2'b11"},
        {m["name"]: m["encoding"] for m in enum["members"]})

      clk_rec = naja.intent_type_of(top.getNet("clk"))
      self.assertEqual("logic", clk_rec["type"])
      self.assertEqual("scalar", clk_rec["canonical_kind"])
      self.assertTrue(clk_rec["src"].endswith("intent_mini.sv:23"))
      self.assertNotIn("enum", clk_rec)
      self.assertNotIn("struct", clk_rec)
      self.assertEqual(clk_rec, naja.intent_type_of(top.getTerm("clk")))

      byte_rec = naja.intent_type_of(top.getNet("byte_q"))
      self.assertEqual("mini_pkg::byte_t", byte_rec["type"])
      self.assertEqual("packed_array", byte_rec["canonical_kind"])
      self.assertTrue(byte_rec["src"].endswith("intent_mini.sv:30"))
      self.assertNotIn("enum", byte_rec)
      self.assertNotIn("struct", byte_rec)

      plain_rec = naja.intent_type_of(top.getNet("plain_q"))
      self.assertEqual("logic[3:0]", plain_rec["type"])
      self.assertEqual("packed_array", plain_rec["canonical_kind"])
      self.assertTrue(plain_rec["src"].endswith("intent_mini.sv:31"))
      self.assertNotIn("enum", plain_rec)
      self.assertNotIn("struct", plain_rec)

      payload_rec = naja.intent_type_of(top.getNet("payload_q"))
      self.assertEqual("mini_pkg::payload_t", payload_rec["type"])
      self.assertEqual("packed_struct", payload_rec["canonical_kind"])
      self.assertTrue(payload_rec["src"].endswith("intent_mini.sv:32"))
      self.assertNotIn("enum", payload_rec)
      payload = payload_rec["struct"]
      self.assertEqual(11, payload["width"])
      self.assertTrue(payload["decl"].endswith("intent_mini.sv:8"))
      self.assertEqual([
        {"name": "valid", "type": "logic", "msb": 10, "lsb": 10},
        {"name": "data", "type": "mini_pkg::byte_t", "msb": 9, "lsb": 2},
        {"name": "state", "type": "mini_pkg::state_e", "msb": 1, "lsb": 0},
      ], payload["fields"])

      overlay_rec = naja.intent_type_of(top.getNet("overlay_q"))
      self.assertEqual("mini_pkg::overlay_t", overlay_rec["type"])
      self.assertEqual("packed_union", overlay_rec["canonical_kind"])
      self.assertEqual(8, overlay_rec["struct"]["width"])
      self.assertTrue(overlay_rec["struct"]["decl"].endswith("intent_mini.sv:13"))
      self.assertEqual([
        {"name": "raw", "type": "logic[7:0]", "msb": 7, "lsb": 0},
        {"name": "data", "type": "mini_pkg::byte_t", "msb": 7, "lsb": 0},
      ], overlay_rec["struct"]["fields"])

      synthetic = naja.SNLScalarNet.create(top, "synthetic")
      self.assertIsNone(naja.intent_type_of(synthetic))
      self.assertIsNone(naja.ast_symbol_of(synthetic))
      self.assertIsNone(naja.intent_parameters_of(synthetic))

      ff = next(
        inst for inst in top.getInstances()
        if inst.getModel().getName() == "naja_dffrn__w2")
      self.assertEqual(type_rec, naja.intent_type_of(ff))

      params = naja.intent_parameters_of(top)
      self.assertEqual("intent_mini", params["module"])
      self.assertEqual(2, params["count"])
      by_name = {p["name"]: p for p in params["parameters"]}
      self.assertFalse(by_name["DEPTH"]["localparam"])
      self.assertEqual("4", by_name["DEPTH"]["value"])
      self.assertEqual("4", by_name["DEPTH"]["expr"])
      self.assertTrue(by_name["IDX_W"]["localparam"])
      self.assertEqual("2", by_name["IDX_W"]["value"])
      self.assertEqual("$clog2(DEPTH)", by_name["IDX_W"]["expr"])

      plen = naja.intent_package_member("mini_pkg", "PLEN")
      self.assertEqual("PLEN", plen["name"])
      self.assertEqual("34", plen["value"])
      self.assertEqual("(32 == 32) ? 34 : 56", plen["expr"])
      self.assertTrue(plen["localparam"])

      package_type = naja.intent_package_member("mini_pkg", "state_e")
      self.assertEqual("mini_pkg::state_e", package_type["type"])
      self.assertEqual("enum", package_type["canonical_kind"])
      self.assertEqual(2, package_type["enum"]["width"])
      self.assertIsNone(naja.intent_package_member("mini_pkg", "missing"))

      state_sym = naja.ast_symbol_of(state_q)
      linked_objects = naja.snl_objects_of(state_sym)
      self.assertIn(state_q, linked_objects)
      self.assertIn(ff, linked_objects)

    db.destroy()
    self.assertFalse(naja.intent_available())

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

    with self.assertRaisesRegex(
        RuntimeError,
        r"applying a destroy\(\) to a Python object with no Hurricane object attached"):
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
