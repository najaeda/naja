# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest

import naja


class SNLLogicalConeTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    designs = naja.NLLibrary.create(db, "designs")
    primitives = naja.NLLibrary.createPrimitives(db, "primitives")

    flop = naja.SNLDesign.createPrimitive(primitives, "DFF")
    self.d = naja.SNLScalarTerm.create(
      flop, naja.SNLTerm.Direction.Input, "D")
    clock = naja.SNLScalarTerm.create(
      flop, naja.SNLTerm.Direction.Input, "CK")
    self.q = naja.SNLScalarTerm.create(
      flop, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLDesign.addInputsToClockArcs(self.d, clock)
    naja.SNLDesign.addClockToOutputsArcs(clock, self.q)

    gate = naja.SNLDesign.createPrimitive(primitives, "AND2")
    self.i0 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Input, "I0")
    self.i1 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Input, "I1")
    self.o = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([self.i0, self.i1], self.o)

    self.top = naja.SNLDesign.create(designs, "TOP")
    self.top_input = naja.SNLScalarTerm.create(
      self.top, naja.SNLTerm.Direction.Input, "IN")
    self.top_output = naja.SNLScalarTerm.create(
      self.top, naja.SNLTerm.Direction.Output, "OUT")
    self.upstream = naja.SNLInstance.create(
      self.top, flop, "upstream")
    self.gate = naja.SNLInstance.create(self.top, gate, "gate")
    self.downstream = naja.SNLInstance.create(
      self.top, flop, "downstream")

    q_net = naja.SNLScalarNet.create(self.top, "q")
    self.upstream.getInstTerm(self.q).setNet(q_net)
    self.gate.getInstTerm(self.i0).setNet(q_net)
    input_net = naja.SNLScalarNet.create(self.top, "input")
    self.top_input.setNet(input_net)
    self.gate.getInstTerm(self.i1).setNet(input_net)
    result_net = naja.SNLScalarNet.create(self.top, "result")
    self.gate.getInstTerm(self.o).setNet(result_net)
    self.downstream.getInstTerm(self.d).setNet(result_net)
    self.top_output.setNet(result_net)

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def test_fanin_and_fanout(self):
    start = naja.SNLOccurrence(self.downstream.getInstTerm(self.d))
    fanin = naja.SNLLogicalCone(
      start, direction=naja.SNLLogicalCone.Direction.FanIn)
    self.assertEqual(naja.SNLLogicalCone.Direction.FanIn,
                     fanin.get_direction())
    self.assertEqual(4, fanin.get_node_count())
    self.assertEqual(2, len(fanin.get_leaves()))
    self.assertEqual(
      {"flop", "ports"}, {node[2] for node in fanin.get_leaves()})

    nodes = fanin.get_nodes()
    root = fanin.get_root()
    self.assertEqual("root", root[2])
    self.assertEqual(1, len(root[3]))
    gate = nodes[root[3][0]]
    self.assertEqual("internal", gate[2])
    self.assertEqual(2, len(gate[3]))
    for leaf_id in gate[3]:
      self.assertEqual((gate[0],), nodes[leaf_id][4])

    fanout = naja.SNLLogicalCone(
      naja.SNLOccurrence(self.upstream.getInstTerm(self.q)),
      naja.SNLLogicalCone.FanOut)
    self.assertEqual(
      {"flop", "ports"}, {node[2] for node in fanout.get_leaves()})

  def test_errors(self):
    bus = naja.SNLBusTerm.create(
      self.top, naja.SNLTerm.Direction.Input, 3, 0, "BUS")
    with self.assertRaises(RuntimeError):
      naja.SNLLogicalCone(naja.SNLOccurrence(), "fanin")
    with self.assertRaises(RuntimeError):
      naja.SNLLogicalCone([naja.SNLOccurrence(self.top_output)],
                          naja.SNLLogicalCone.FanIn)
    with self.assertRaises(RuntimeError):
      naja.SNLLogicalCone(
        naja.SNLOccurrence(self.gate), naja.SNLLogicalCone.FanIn)
    with self.assertRaises(RuntimeError):
      naja.SNLLogicalCone(
        naja.SNLOccurrence(bus), naja.SNLLogicalCone.FanIn)
    with self.assertRaises(RuntimeError):
      naja.SNLLogicalCone(
        naja.SNLOccurrence(self.top_output), "sideways")


if __name__ == "__main__":
  unittest.main()
