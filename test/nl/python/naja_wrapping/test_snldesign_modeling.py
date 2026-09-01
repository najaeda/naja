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

  def testTermRoleDecoration(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    clock = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Input, "CLK")
    data = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Input, "D")
    reset = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Input, "RESET_B")
    output = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "Q")

    clock.setRole(naja.SNLTermRole.Clock)
    data.setRole(naja.SNLTermRole.DataInput)
    reset.setRole(naja.SNLTermRole.AsyncReset, naja.SNLActiveLevel.Low)
    output.setRole(naja.SNLTermRole.DataOutput)
    for role in (
        naja.SNLTermRole.MemoryWriteAddress,
        naja.SNLTermRole.Other,
        naja.SNLTermRole.ScanInput,
        naja.SNLTermRole.ScanEnable):
      data.setRole(role)
      self.assertEqual(role, data.getRole())
    data.setRole(naja.SNLTermRole.DataInput)

    self.assertEqual(naja.SNLTermRole.Clock, clock.getRole())
    self.assertEqual(naja.SNLTermRole.DataInput, data.getRole())
    self.assertEqual(naja.SNLTermRole.AsyncReset, reset.getRole())
    self.assertEqual(naja.SNLActiveLevel.Low, reset.getResetActiveLevel())
    self.assertEqual(naja.SNLTermRole.DataOutput, output.getRole())
    self.assertEqual([clock], list(reg.getClockTerms()))
    self.assertEqual([reset], list(reg.getAsyncResetTerms()))
    self.assertEqual([data], list(reg.getDataInputTerms()))
    self.assertEqual([output], list(reg.getOutputTerms()))

    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, reg, "reg")
    self.assertTrue(instance.getInstTerm(clock).is_clock())
    self.assertTrue(instance.getInstTerm(data).is_data_input())
    self.assertTrue(instance.getInstTerm(reset).is_async_reset())
    self.assertEqual(
      naja.SNLActiveLevel.Low,
      instance.getInstTerm(reset).getResetActiveLevel())
    self.assertTrue(instance.getInstTerm(output).is_data_output())

    with self.assertRaises(RuntimeError):
      clock.setRole()
    with self.assertRaises(RuntimeError):
      clock.setRole("Clock")
    with self.assertRaises(RuntimeError):
      clock.setRole(1000)
    with self.assertRaises(RuntimeError):
      clock.setRole(naja.SNLTermRole.Clock, "High")
    with self.assertRaises(RuntimeError):
      clock.setRole(naja.SNLTermRole.Clock, 1000)

  def testSequentialModel(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "SCAN_REG")
    q = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "Q")
    qn = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "QN")
    for name in ("CLK", "D", "SE", "SI", "RESET_B"):
      naja.SNLScalarTerm.create(
        reg, naja.SNLTerm.Direction.Input, name)

    self.assertFalse(reg.hasSequentialModel())
    reg.setSequentialModel(
      clocked_on="CLK",
      states=[{
        "name": "IQ",
        "inverted_name": "IQN",
        "next_state": "(SE & SI) | (!SE & D)",
        "clear": "!RESET_B",
      }],
      outputs=[(q, "IQ"), (qn, "IQN")])
    self.assertTrue(reg.hasSequentialModel())

  def testSequentialModelOutputErrors(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    q = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "Q")
    for name in ("CLK", "D"):
      naja.SNLScalarTerm.create(
        reg, naja.SNLTerm.Direction.Input, name)
    states = [{"name": "IQ", "next_state": "D"}]

    def set_outputs(outputs):
      reg.setSequentialModel(
        clocked_on="CLK", states=states, outputs=outputs)

    with self.assertRaisesRegex(
        RuntimeError, "expects lists for states and outputs"):
      set_outputs(((q, "IQ"),))
    for outputs in ([q], [(q,)]):
      with self.assertRaisesRegex(
          RuntimeError,
          r"outputs must be \(SNLBitTerm, expression\) tuples"):
        set_outputs(outputs)
    for outputs in ([("Q", "IQ")], [(q, 0)]):
      with self.assertRaisesRegex(
          RuntimeError,
          r"outputs must be \(SNLBitTerm, expression\) tuples"):
        set_outputs(outputs)

    foreign = naja.SNLDesign.createPrimitive(self.primitives, "FOREIGN")
    foreign_q = naja.SNLScalarTerm.create(
      foreign, naja.SNLTerm.Direction.Output, "Q")
    with self.assertRaisesRegex(
        RuntimeError, "output term belongs to another design"):
      set_outputs([(foreign_q, "IQ")])

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
    self.assertEqual([c], list(naja.SNLDesign.getInputRelatedClocks(d0)))
    self.assertEqual([c], list(naja.SNLDesign.getInputRelatedClocks(d1)))
    self.assertEqual([c], list(naja.SNLDesign.getOutputRelatedClocks(q0)))
    self.assertEqual([c], list(naja.SNLDesign.getOutputRelatedClocks(q1)))

    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, reg, "reg")
    ic = instance.getInstTerm(c)
    id0 = instance.getInstTerm(d0)
    iq0 = instance.getInstTerm(q0)
    self.assertEqual(
      [id0, instance.getInstTerm(d1)],
      list(instance.getClockRelatedInputs(ic)))
    self.assertEqual(
      [iq0, instance.getInstTerm(q1)],
      list(instance.getClockRelatedOutputs(ic)))
    self.assertEqual([ic], list(instance.getInputRelatedClocks(id0)))
    self.assertEqual([ic], list(instance.getOutputRelatedClocks(iq0)))

  def testParameterizedCombinatorialArcs(self):
    gate = naja.SNLDesign.createPrimitive(self.primitives, "PARAM_GATE")
    mode = naja.SNLParameter.create_string(gate, "MODE", "NORMAL")
    i0 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Input, "I1")
    o0 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Output, "O0")
    o1 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Output, "O1")

    gate.setTimingModelParameter("MODE", "NORMAL")
    naja.SNLDesign.addCombinatorialArcs(i0, o0)
    naja.SNLDesign.addCombinatorialArcs(i1, o1)
    naja.SNLDesign.addCombinatorialArcs("CROSS", i0, o1)
    naja.SNLDesign.addCombinatorialArcs("CROSS", i1, o0)

    self.assertEqual([o0], list(gate.getCombinatorialOutputs(i0)))
    self.assertEqual([o1], list(gate.getCombinatorialOutputs(i1)))

    top = naja.SNLDesign.create(self.designs, "TOP")
    normal = naja.SNLInstance.create(top, gate, "normal")
    cross = naja.SNLInstance.create(top, gate, "cross")
    naja.SNLInstParameter.create(cross, mode, "CROSS")
    self.assertEqual(
      normal.getInstTerm(o0),
      next(iter(normal.getCombinatorialOutputs(normal.getInstTerm(i0)))))
    self.assertEqual(
      cross.getInstTerm(o1),
      next(iter(cross.getCombinatorialOutputs(cross.getInstTerm(i0)))))

    with self.assertRaises(RuntimeError):
      gate.setTimingModelParameter("UNKNOWN", "NORMAL")
    with self.assertRaises(RuntimeError):
      gate.setTimingModelParameter("MODE")
    with self.assertRaises(RuntimeError):
      naja.SNLDesign.addCombinatorialArcs(1, i0, o0)

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
