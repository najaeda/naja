# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest
import naja

class SNLDesignModelingTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    self.db = naja.NLDB.create(universe)
    self.designs = naja.NLLibrary.create(self.db)
    self.primitives = naja.NLLibrary.createPrimitives(self.db)

  def tearDown(self):
    del self.designs
    del self.primitives
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testLibraries(self):
    self.assertTrue(self.designs.isStandard())
    self.assertFalse(self.designs.isPrimitives())
    self.assertTrue(self.primitives.isPrimitives())
    self.assertFalse(self.primitives.isStandard())

  def testLoweredSequentialTermRoles(self):
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    async_path = os.path.join(
      formats_path, "systemverilog", "benchmarks",
      "seq_timing_event_list_negedge_reset_supported",
      "seq_timing_event_list_negedge_reset_supported.sv")
    top = self.db.loadSystemVerilog([async_path])
    dffrn_inst = next(
      inst for inst in top.getPrimitiveInstances()
      if inst.getModel().getName().startswith("naja_dffrn"))
    dffrn = dffrn_inst.getModel()

    clocks = list(dffrn.getClockTerms())
    resets = list(dffrn.getAsyncResetTerms())
    data_inputs = list(dffrn.getDataInputTerms())
    outputs = list(dffrn.getOutputTerms())
    self.assertEqual(1, len(clocks))
    self.assertEqual(1, len(resets))
    self.assertEqual(8, len(data_inputs))
    self.assertEqual(8, len(outputs))
    self.assertEqual(naja.SNLTermRole.Clock, clocks[0].getRole())
    self.assertTrue(clocks[0].is_clock())
    self.assertEqual(naja.SNLTermRole.AsyncReset, resets[0].getRole())
    self.assertEqual(naja.SNLActiveLevel.Low, resets[0].getResetActiveLevel())
    self.assertTrue(resets[0].is_async_reset())
    self.assertTrue(resets[0].is_reset())
    self.assertFalse(resets[0].is_data())
    self.assertEqual(9, len(list(naja.SNLDesign.getClockRelatedInputs(clocks[0]))))
    self.assertTrue(all(term.is_data_input() for term in data_inputs))
    self.assertTrue(all(term.is_data_output() for term in outputs))
    self.assertTrue(data_inputs[0].is_data())
    self.assertTrue(outputs[0].is_data())

    reset_inst_term = next(
      term for term in dffrn_inst.getInstTerms() if term.is_async_reset())
    self.assertEqual(naja.SNLTermRole.AsyncReset, reset_inst_term.getRole())
    self.assertEqual(
      naja.SNLActiveLevel.Low, reset_inst_term.getResetActiveLevel())
    self.assertTrue(reset_inst_term.is_reset())
    self.assertFalse(reset_inst_term.is_async_set())
    self.assertFalse(reset_inst_term.is_data())

    clock_inst_term = next(
      term for term in dffrn_inst.getInstTerms() if term.is_clock())
    data_input_inst_term = next(
      term for term in dffrn_inst.getInstTerms() if term.is_data_input())
    data_output_inst_term = next(
      term for term in dffrn_inst.getInstTerms() if term.is_data_output())
    self.assertTrue(data_input_inst_term.is_data())
    self.assertTrue(data_output_inst_term.is_data())
    self.assertFalse(clock_inst_term.is_enable())
    self.assertFalse(clock_inst_term.is_data())
    self.assertFalse(data_input_inst_term.is_clock())
    self.assertFalse(data_input_inst_term.is_data_output())
    self.assertFalse(data_output_inst_term.is_data_input())

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sv", delete=False) as async_set_file:
      async_set_file.write("""
module py_async_set_role_coverage(
  input logic clk,
  input logic set,
  input logic d,
  output logic q
);
  always_ff @(posedge clk or posedge set) begin
    if (set) q <= 1'b1;
    else q <= d;
  end
endmodule
""")
      async_set_path = async_set_file.name
    try:
      async_set_db = naja.NLDB.create(naja.NLUniverse.get())
      async_set_top = async_set_db.loadSystemVerilog([async_set_path])
      dffs_inst = next(
        inst for inst in async_set_top.getPrimitiveInstances()
        if inst.getModel().getName().startswith("naja_dffs"))
      dffs = dffs_inst.getModel()
      sets = list(dffs.getAsyncSetTerms())
      self.assertEqual(1, len(sets))
      self.assertEqual(naja.SNLTermRole.AsyncSet, sets[0].getRole())
      set_inst_term = next(
        term for term in dffs_inst.getInstTerms() if term.is_async_set())
      set_bit_term = set_inst_term.getBitTerm()
      self.assertEqual(naja.SNLTermRole.AsyncSet, set_inst_term.getRole())
      self.assertEqual(sets[0], set_bit_term)
      self.assertFalse(set_inst_term.is_reset())
      self.assertTrue(set_bit_term.is_async_set())
      self.assertFalse(set_bit_term.is_reset())
    finally:
      os.remove(async_set_path)

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sv", delete=False) as enable_file:
      enable_file.write("""
module py_enable_role_coverage(
  input logic clk,
  input logic en,
  input logic d,
  output logic q
);
  always_ff @(posedge clk) begin
    if (en) q <= d;
    else q <= q;
  end
endmodule
""")
      enable_path = enable_file.name
    try:
      enable_db = naja.NLDB.create(naja.NLUniverse.get())
      enable_top = enable_db.loadSystemVerilog([enable_path])
      dffe_inst = next(
        inst for inst in enable_top.getPrimitiveInstances()
        if inst.getModel().getName().startswith("naja_dffe"))
      enable_bit_term = next(
        term for term in dffe_inst.getModel().getBitTerms()
        if term.is_enable())
      self.assertEqual(naja.SNLTermRole.Enable, enable_bit_term.getRole())
      self.assertTrue(enable_bit_term.is_enable())
      self.assertFalse(enable_bit_term.is_data())
    finally:
      os.remove(enable_path)

    sync_path = os.path.join(
      formats_path, "systemverilog", "benchmarks",
      "seq_reset_action_supported", "seq_reset_action_supported.sv")
    sync_db = naja.NLDB.create(naja.NLUniverse.get())
    sync_top = sync_db.loadSystemVerilog([sync_path])
    dff = next(
      inst.getModel() for inst in sync_top.getPrimitiveInstances()
      if inst.getModel().getName().startswith("naja_dff__"))
    self.assertEqual([], list(dff.getAsyncResetTerms()))
    self.assertTrue(all(
      term.getRole() != naja.SNLTermRole.AsyncReset
      for term in dff.getBitTerms()))

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sv", delete=False) as sync_reset_file:
      sync_reset_file.write("""
module py_sync_reset_role_coverage(
  input logic clk,
  input logic rst,
  input logic [1:0] d,
  output logic [1:0] q
);
  always_ff @(posedge clk) begin
    if (rst) q <= 2'b00;
    else q <= d;
  end
endmodule
""")
      sync_reset_path = sync_reset_file.name
    try:
      sync_reset_db = naja.NLDB.create(naja.NLUniverse.get())
      sync_reset_top = sync_reset_db.loadSystemVerilog([sync_reset_path])
      dffsr_inst = next(
        inst for inst in sync_reset_top.getPrimitiveInstances()
        if inst.getModel().getName().startswith("naja_dffsr"))
      dffsr = dffsr_inst.getModel()
      sync_resets = list(dffsr.getSyncResetTerms())
      self.assertEqual(1, len(sync_resets))
      self.assertEqual(naja.SNLTermRole.SyncReset, sync_resets[0].getRole())
      self.assertEqual(
        naja.SNLActiveLevel.High, sync_resets[0].getResetActiveLevel())
      self.assertTrue(sync_resets[0].is_sync_reset())
      self.assertTrue(sync_resets[0].is_reset())
      self.assertEqual([], list(dffsr.getAsyncResetTerms()))
      sync_reset_inst_term = next(
        term for term in dffsr_inst.getInstTerms() if term.is_sync_reset())
      self.assertEqual(naja.SNLTermRole.SyncReset, sync_reset_inst_term.getRole())
      self.assertTrue(sync_reset_inst_term.is_reset())
      self.assertFalse(sync_reset_inst_term.is_async_reset())
    finally:
      os.remove(sync_reset_path)

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sv", delete=False) as sync_set_file:
      sync_set_file.write("""
module py_sync_set_role_coverage(
  input logic clk,
  input logic set,
  input logic en,
  input logic [2:0] d,
  output logic [2:0] q
);
  always_ff @(posedge clk) begin
    if (set) q <= 3'b111;
    else if (en) q <= d;
  end
endmodule
""")
      sync_set_path = sync_set_file.name
    try:
      sync_set_db = naja.NLDB.create(naja.NLUniverse.get())
      sync_set_top = sync_set_db.loadSystemVerilog([sync_set_path])
      dffsse_inst = next(
        inst for inst in sync_set_top.getPrimitiveInstances()
        if inst.getModel().getName().startswith("naja_dffsse"))
      dffsse = dffsse_inst.getModel()
      sync_sets = list(dffsse.getSyncSetTerms())
      self.assertEqual(1, len(sync_sets))
      self.assertEqual(naja.SNLTermRole.SyncSet, sync_sets[0].getRole())
      self.assertEqual(
        naja.SNLActiveLevel.High, sync_sets[0].getResetActiveLevel())
      self.assertTrue(sync_sets[0].is_sync_set())
      self.assertFalse(sync_sets[0].is_reset())
      self.assertFalse(sync_sets[0].is_sync_reset())
      sync_set_inst_term = next(
        term for term in dffsse_inst.getInstTerms() if term.is_sync_set())
      self.assertEqual(naja.SNLTermRole.SyncSet, sync_set_inst_term.getRole())
      self.assertTrue(sync_set_inst_term.is_sync_set())
      self.assertFalse(sync_set_inst_term.is_reset())
      self.assertFalse(sync_set_inst_term.is_sync_reset())
    finally:
      os.remove(sync_set_path)

  def testCombi(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    i0 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([i0, i1], o)
    self.assertTrue(design.isPrimitive())
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(i0)))
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(i1)))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o)))
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(o)))
    inputs = [t for t in naja.SNLDesign.getCombinatorialInputs(o)]
    self.assertEqual(2, len(inputs))
    self.assertEqual(i0, inputs[0])
    self.assertEqual(i1, inputs[1])
    outputs = [t for t in naja.SNLDesign.getCombinatorialOutputs(i0)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])
    outputs = [t for t in naja.SNLDesign.getCombinatorialOutputs(i1)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])

    #create instance
    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, design, "instance")
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i0))))
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i1))))
    self.assertEqual(2, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))))
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(o))))
    inputs = [it for it in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))]
    self.assertEqual(2, len(inputs))
    self.assertEqual(instance.getInstTerm(i0), inputs[0])
    self.assertEqual(instance.getInstTerm(i1), inputs[1])
    
    self.assertEqual(1, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i0))))
    outputs = [t for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i0))]
    self.assertEqual(1, len(outputs))
    self.assertEqual(instance.getInstTerm(o), outputs[0])
    self.assertEqual(1, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i1))))
    outputs = [t for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i1))]
    self.assertEqual(1, len(outputs))
    self.assertEqual(instance.getInstTerm(o), outputs[0])

  def testSeq(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d0 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "D0")
    q0 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Output, "Q0")
    d1 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "D1")
    q1 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Output, "Q1")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs(d0, c)
    naja.SNLDesign.addClockToOutputsArcs(c, q0)
    naja.SNLDesign.addInputsToClockArcs([d1], c)
    naja.SNLDesign.addClockToOutputsArcs(c, [q1])
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))

  def testCombiWithBusses0(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    o = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Output, 3, 0, "O")
    d = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Input, 3, 0, "D")
    naja.SNLDesign.addCombinatorialArcs(d, o)
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o.getBusTermBit(0))))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(d.getBusTermBit(0))))

  def testCombiWithBusses1(self):
    carry4 = naja.SNLDesign.createPrimitive(self.primitives, "CARRY4")
    o = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Output, 3, 0, "O")
    co = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Output, 3, 0, "CO")
    di = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Input, 3, 0, "DI")
    s = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Input, 3, 0, "S")
    cyinit  = naja.SNLScalarTerm.create(carry4, naja.SNLTerm.Direction.Input, "CYINIT")
    ci = naja.SNLScalarTerm.create(carry4, naja.SNLTerm.Direction.Input, "CI")
    o_bits = [b for b in o.getBits()]
    co_bits = [b for b in co.getBits()]
    di_bits = [b for b in di.getBits()] 
    s_bits = [b for b in s.getBits()] 
    #cyinit and ci are in combinatorial dependency with o and co outputs 
    naja.SNLDesign.addCombinatorialArcs([cyinit, ci], [o, co])
    naja.SNLDesign.addCombinatorialArcs(s_bits[0], [o, co])
    naja.SNLDesign.addCombinatorialArcs(s_bits[1], [o_bits[1], o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[2], [o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[2], [co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[3], o_bits[3])
    naja.SNLDesign.addCombinatorialArcs(s_bits[3], co_bits[3])
    naja.SNLDesign.addCombinatorialArcs(di_bits[0], [o_bits[1], o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[0], co)
    naja.SNLDesign.addCombinatorialArcs(di_bits[1], [o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[2], o_bits[3])
    naja.SNLDesign.addCombinatorialArcs(di_bits[2], [co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[3], co_bits[3])
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(cyinit)))
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(ci)))
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[0])))
    self.assertEqual(6, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[1])))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[2])))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[3])))
    self.assertEqual(3, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[0])))
    self.assertEqual(5, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[1])))
    self.assertEqual(7, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[2])))
    self.assertEqual(9, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[3])))

  def testCombiWithBusses2(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    o = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Output, 3, 0, "O")
    i = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Input, 3, 0, "I")
    naja.SNLDesign.addCombinatorialArcs([i], [o])
    for o_bit in o.getBits():
      self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bit)))
    for i_bit in i.getBits():
      self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(i_bit)))

  def testSeqWithBusses0(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Input, 3, 0, "D")
    q = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Output, 3, 0, "Q")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs(d, c)
    naja.SNLDesign.addClockToOutputsArcs(c, q)
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))

  def testSeqWithBusses1(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Input, 3, 0, "D")
    q = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Output, 3, 0, "Q")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs([d], c)
    naja.SNLDesign.addClockToOutputsArcs(c, [q])
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))

  def testCreationErrors(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "design")
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.createPrimitive(self.designs, "design")

  def testCombiErrors(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    i0 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "O")
    #wrong type
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(i0, i1, o)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(design, o)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(i0, design)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs([design, i0], [o, design])
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs([i0], [o, design])

  def testSeqErrors(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    d = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "D")
    q = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "Q")
    c = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "C")
    #wrong type
    naja.SNLDesign.addClockToOutputsArcs(c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(d, c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(d, [c, q])
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(design, c)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs([design], c)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(d, c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs([d, c], q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(c, design)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(c, [design])
    with self.assertRaises(RuntimeError) as context: design.getCombinatorialInputs(design)
    with self.assertRaises(RuntimeError) as context: design.getCombinatorialOutputs(design)

    #create instance
    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, design, "instance")
    with self.assertRaises(RuntimeError) as context: instance.getCombinatorialInputs(d)
    with self.assertRaises(RuntimeError) as context: instance.getCombinatorialOutputs(q)
   
if __name__ == '__main__':
  unittest.main()
