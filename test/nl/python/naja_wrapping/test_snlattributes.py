# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import sys
import tempfile
import os
import unittest
import naja

class SNLAttributesTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    lib = naja.NLLibrary.create(db)
    self.top = naja.SNLDesign.create(lib)
    model = naja.SNLDesign.create(lib)
    naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, "I0")
    naja.SNLBusTerm.create(model, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Output, "O")
    self.ins0 = naja.SNLInstance.create(self.top, model, "ins0")
    self.ins1 = naja.SNLInstance.create(self.top, model, "ins1")

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testSourceReferences(self):
    model = self.ins0.getModel()
    net = naja.SNLScalarNet.create(self.top, "source_net")
    bus = naja.SNLBusNet.create(self.top, 3, 0, "source_bus")
    term = model.getTerm("I0")
    objects = (self.top, self.ins0, net, bus, bus.getBit(1), term,
               model.getTerm("I1").getBusTermBit(2), self.ins0.getInstTerm(term))
    for obj in objects:
      self.assertIsNone(obj.getSourceDeclaration())
      self.assertEqual([], obj.getSourceOrigins())
      self.assertFalse(obj.hasSourceReferences())
      with self.assertRaises(TypeError):
        obj.hasSourceReferences(False)
      for name in ("setSourceLoc", "setSourceDeclaration", "addSourceOrigin", "clearSourceOrigins"):
        self.assertFalse(hasattr(obj, name))

  def testSourceReferencesVerilog(self):
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "input.v")
      with open(path, "w", encoding="utf-8") as output:
        output.write('module cell(input a, output y); endmodule\n'
                     '(* src="rtl.sv:1.70000-2.90000" *) module top(input a, output y);\n'
                     '(* src="rtl.sv:3.1-3.9" *) cell u(a,y); endmodule\n')
      db = self.top.getDB()
      db.loadGateLevelVerilog([path], yosys_source_attributes=True)
      top = db.getTopDesign()
      objects = (top, top.getTerm("a"), top.getNet("a"), top.getInstance("u"))
      before = [(obj.getSourceDeclaration(), obj.getSourceOrigins()) for obj in objects]
      for mode in ("CompactAttribute", "VerboseAttributes"):
        top.dumpGateLevelVerilog(directory, "source.v", dumpRTLInfosAsAttributes=True,
                        rtlInfoDumpMode=mode)
        with open(os.path.join(directory, "source.v"), encoding="utf-8") as output:
          exported = output.read()
        self.assertNotIn("naja_source_v1", exported)
        self.assertNotIn("naja_port_net_source_v1", exported)
      self.assertEqual(before, [(obj.getSourceDeclaration(), obj.getSourceOrigins())
                                for obj in objects])

  def testSourceOriginsInvalidUtf8(self):
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "input.v")
      with open(path, "w", encoding="utf-8") as output:
        output.write(r'(* src="valid.sv:1|invalid\377.sv:2" *) module cell(); endmodule')
      db = self.top.getDB()
      db.loadGateLevelVerilog([path], yosys_source_attributes=True)
      top = db.getTopDesign()
      # The first origin converts successfully before the second fails.
      for _ in range(2):
        with self.assertRaises(UnicodeDecodeError):
          top.getSourceOrigins()
      self.assertEqual(path, top.getSourceDeclaration()[0][0])

  def testCustomSourceAttributesStayOpaque(self):
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "opaque.v")
      with open(path, "w", encoding="utf-8") as output:
        output.write('(* naja_source_v1=42, src="rtl.sv:3.1-3.9" *) '
                     'module opaque((* naja_port_net_source_v1="arbitrary" *) input a); endmodule')
      db = self.top.getDB()
      db.loadGateLevelVerilog([path], yosys_source_attributes=True)
      top = db.getTopDesign()
      self.assertEqual(1, len(top.getSourceOrigins()))
      self.assertEqual("rtl.sv", top.getSourceOrigins()[0][0][0])
      self.assertEqual("42", next(a.getValue() for a in top.getAttributes()
                                  if a.getName() == "naja_source_v1"))
      self.assertEqual("arbitrary", next(a.getValue() for a in top.getTerm("a").getAttributes()
                                         if a.getName() == "naja_port_net_source_v1"))
      self.assertEqual([], top.getNet("a").getSourceOrigins())

  def testYosysSourcePopulation(self):
    text = r"""(* src = "definition.sv:1.2-9.4" *)
module leaf(input a, output y); endmodule
module synthesized(input a, output y);
(* src = "C:\\rtl\\a.sv:12.70000-12.80000|other:with:colon.sv:4.1-5.2|C:\\rtl\\a.sv:12.70000-12.80000|partial:7|zero:0.0-0.0|quoted\"file.sv:3.1-3.2|bad|invalid\q:1.1|octal\777:1.1|nul\000:1.1|over:1.4294967296-2.1|reverse:9.1-8.2|" *)
leaf u(a, y);
(* src *) wire missing;
(* src = 42 *) wire numeric;
(* src = "" *) wire empty;
endmodule
"""
    expected = [((r"C:\rtl\a.sv", 12, 70000, 12, 80000), "yosys", "", ""),
                (("other:with:colon.sv", 4, 1, 5, 2), "yosys", "", ""),
                (("partial", 7, 0, 0, 0), "yosys", "", ""),
                (("zero", 0, 0, 0, 0), "yosys", "", ""),
                (('quoted"file.sv', 3, 1, 3, 2), "yosys", "", "")]
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "gates.v")
      with open(path, "w", encoding="utf-8") as output:
        output.write(text)
      # Unselected generic src attributes stay opaque.
      db = self.top.getDB()
      db.loadGateLevelVerilog([path])
      top = db.getTopDesign()
      self.assertEqual([], top.getInstance("u").getSourceOrigins())
      naja.NLUniverse.get().destroy()
      db = naja.NLDB.create(naja.NLUniverse.create())
      db.loadGateLevelVerilog([path], yosys_source_attributes=True)
      top = db.getTopDesign()
      instance = top.getInstance("u")
      self.assertEqual(expected, instance.getSourceOrigins())
      self.assertEqual((path, 5, 6, 5, 6), instance.getSourceDeclaration()[0])
      self.assertEqual(2, instance.getModel().getSourceDeclaration()[0][1])
      self.assertIsNone(instance.getSourceLoc())
      for name in ("missing", "numeric", "empty"):
        self.assertEqual([], top.getNet(name).getSourceOrigins())
        self.assertTrue(top.getNet(name).getSourceDeclaration())
      raw_src = next(a.getValue() for a in instance.getAttributes() if a.getName() == "src")
      self.assertIn("|bad|", raw_src)
      # NajaIF retains roles; Verilog retains raw src attributes for explicit decoding.
      snapshot = os.path.join(directory, "snapshot")
      db.dumpNajaIF(snapshot)
      top.dumpGateLevelVerilog(directory, "roundtrip.v", dumpRTLInfosAsAttributes=True)
      os.unlink(path)
      for kind in ("snapshot", "verilog"):
        naja.NLUniverse.get().destroy()
        if kind == "snapshot":
          db = naja.NLDB.loadNajaIF(snapshot)
        else:
          db = naja.NLDB.create(naja.NLUniverse.create())
          db.loadGateLevelVerilog([os.path.join(directory, "roundtrip.v")], yosys_source_attributes=True)
        instance = db.getTopDesign().getInstance("u")
        self.assertEqual(expected, instance.getSourceOrigins())
        if kind == "snapshot":
          self.assertEqual((path, 5, 6, 5, 6), instance.getSourceDeclaration()[0])
        else:
          self.assertEqual(os.path.join(directory, "roundtrip.v"), instance.getSourceDeclaration()[0][0])
        if kind == "verilog":
          self.assertEqual(raw_src, next(a.getValue() for a in instance.getAttributes() if a.getName() == "src"))

  def testPreprocessedSourceDoesNotFabricateDeclarations(self):
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "preprocessed.v")
      with open(path, "w", encoding="utf-8") as output:
        output.write('(* src="rtl.sv:5.1-5.2" *) module expanded(input a); endmodule')
      db = self.top.getDB()
      db.loadGateLevelVerilog([path], preprocess_enabled=True, yosys_source_attributes=True)
      top = db.getTopDesign()
      self.assertIsNone(top.getSourceDeclaration())
      self.assertIsNone(top.getTerm("a").getSourceDeclaration())
      self.assertEqual([(("rtl.sv", 5, 1, 5, 2), "yosys", "", "")], top.getSourceOrigins())

  def testAttributes(self):
    pragma0 = naja.SNLAttribute("pragma0", "value0")
    self.assertEqual("pragma0", pragma0.getName())
    self.assertEqual("value0", pragma0.getValue())
    self.assertTrue(pragma0.hasValue())
    pragma1 = naja.SNLAttribute("pragma1", 10)
    self.assertEqual("pragma1", pragma1.getName())
    self.assertEqual("10", pragma1.getValue())
    self.assertTrue(pragma1.hasValue())
    pragma2 = naja.SNLAttribute("pragma2")
    self.assertEqual("pragma2", pragma2.getName())
    self.assertEqual("", pragma2.getValue())
    self.assertFalse(pragma2.hasValue())
    self.assertEqual(hash(pragma0), hash(naja.SNLAttribute("pragma0", "value0")))
    self.assertEqual(1, len({pragma0, naja.SNLAttribute("pragma0", "value0")}))
    self.assertEqual("pragma0", {pragma0: "pragma0"}[naja.SNLAttribute("pragma0", "value0")])
    self.assertNotEqual(naja.SNLAttribute("pragma1", "10"), pragma1)
    self.assertEqual(2, len({pragma1, naja.SNLAttribute("pragma1", "10")}))
    unbound = naja.SNLAttribute.__new__(naja.SNLAttribute)
    with self.assertRaisesRegex(
      RuntimeError,
      r"Attempt to call SNLAttribute\.__hash__\(\) on an unbound object"):
      hash(unbound)

  def testAttributesErrors(self):
    with self.assertRaises(RuntimeError) as context: naja.SNLAttribute("pragma0", "value0", "value1")
    with self.assertRaises(RuntimeError) as context: naja.SNLAttribute(3, "value0")
    with self.assertRaises(RuntimeError) as context: naja.SNLAttribute("pragma0", self.ins0)

  def test(self):
    self.assertFalse(self.top.hasSourceLoc())
    self.assertIsNone(self.top.getSourceLoc())
    self.assertFalse(self.ins0.hasSourceLoc())
    self.assertIsNone(self.ins0.getSourceLoc())

    self.assertEqual(0, sum(1 for a in self.ins0.getAttributes()))
    self.ins0.addAttribute(naja.SNLAttribute("attr0", "value0"))
    self.assertEqual(1, sum(1 for a in self.ins0.getAttributes()))
    self.ins0.addAttribute(naja.SNLAttribute("attr1", "value1"))
    self.assertEqual(2, sum(1 for a in self.ins0.getAttributes()))
    attributes = list(self.ins0.getAttributes())
    self.assertEqual("attr0", attributes[0].getName())
    self.assertEqual("value0", attributes[0].getValue())
    self.assertEqual("attr1", attributes[1].getName())
    self.assertEqual("value1", attributes[1].getValue())
    #add and test on top
    self.assertEqual(0, sum(1 for a in self.top.getAttributes()))
    self.top.addAttribute(naja.SNLAttribute("topattr", "topvalue"))
    self.assertEqual(1, sum(1 for a in self.top.getAttributes()))
    attributes = list(self.top.getAttributes())
    self.assertEqual("topattr", attributes[0].getName())
    self.assertEqual("topvalue", attributes[0].getValue())

  def testErrors(self):
    with self.assertRaises(RuntimeError) as context: self.ins0.addAttribute(3)
    with self.assertRaises(RuntimeError) as context: self.top.addAttribute(3)
  
if __name__ == '__main__':
  unittest.main()
