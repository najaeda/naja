// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TClockPropagator.h"

#include <stdexcept>
#include <vector>

#include "TClock.h"
#include "TEdge.h"
#include "TGraph.h"
#include "TNode.h"

namespace naja::TG {

void TClockPropagator::propagateForward(TGraph& graph) {
  const std::size_t nodesCount = graph.getNodesStorage().size();
  const std::size_t clocksCount = graph.getClocksCount();

  //resolved[n] = clocks reaching n by following enabled edges forward
  //from their seed (see class doc for the exact definition).
  std::vector<TEdge::ClockMask> resolved(nodesCount, TEdge::ClockMask(clocksCount));
  for (std::size_t c = 0; c < clocksCount; ++c) {
    const TClock* clock = graph.getClock(c);
    resolved[clock->getNode()].set(c);
  }

  //Kahn's algorithm over enabled edges only, so a cycle broken by
  //TLoopBreaker (a disabled back edge) does not block topological order.
  std::vector<std::size_t> inDegree(nodesCount, 0);
  for (TGraph::NodeId n = 0; n < nodesCount; ++n) {
    if (!graph.isNodeValid(n)) {
      continue;
    }
    for (TGraph::EdgeId eid : graph.getNode(n)->getInEdges()) {
      const TEdge* edge = graph.getEdge(eid);
      if (edge != nullptr && !edge->isDisabled()) {
        ++inDegree[n];
      }
    }
  }

  std::vector<TGraph::NodeId> queue;
  for (TGraph::NodeId n = 0; n < nodesCount; ++n) {
    if (graph.isNodeValid(n) && inDegree[n] == 0) {
      queue.push_back(n);
    }
  }

  std::size_t processedCount = 0;
  for (std::size_t qi = 0; qi < queue.size(); ++qi) {
    const TGraph::NodeId n = queue[qi];
    ++processedCount;
    for (TGraph::EdgeId eid : graph.getNode(n)->getOutEdges()) {
      TEdge* edge = graph.getEdge(eid);
      if (edge == nullptr || edge->isDisabled()) {
        continue;
      }
      const TGraph::NodeId m = edge->getTarget();
      resolved[m] |= resolved[n];
      if (--inDegree[m] == 0) {
        queue.push_back(m);
      }
    }
  }

  if (processedCount != graph.getNodesCount()) {
    throw std::logic_error(
      "TClockPropagator::propagateForward: cycle among enabled edges - run TLoopBreaker first");
  }

  for (TGraph::NodeId n = 0; n < nodesCount; ++n) {
    if (!graph.isNodeValid(n)) {
      continue;
    }
    for (TGraph::EdgeId eid : graph.getNode(n)->getOutEdges()) {
      TEdge* edge = graph.getEdge(eid);
      if (edge == nullptr || edge->isDisabled()) {
        continue;
      }
      edge->setFromClocks(resolved[n]);
    }
  }
}

void TClockPropagator::propagateBackward(TGraph& graph) {
  const std::size_t nodesCount = graph.getNodesStorage().size();
  const std::size_t clocksCount = graph.getClocksCount();

  //A node's own seed bits are part of its resolved mask too - propagateForward
  //folds them in via `resolved`, but that array is local to that function, so
  //recompute it here rather than assuming a node's in-edges alone account for
  //it (a node can be a TClock seed AND have in-edges from other clocks).
  std::vector<TEdge::ClockMask> seedMask(nodesCount, TEdge::ClockMask(clocksCount));
  for (std::size_t c = 0; c < clocksCount; ++c) {
    const TClock* clock = graph.getClock(c);
    seedMask[clock->getNode()].set(c);
  }

  for (TGraph::NodeId n = 0; n < nodesCount; ++n) {
    if (!graph.isNodeValid(n)) {
      continue;
    }
    //Aggregate what forward propagation resolved for n across every path
    //that reaches it, not just one edge's own contribution.
    TEdge::ClockMask atNode = seedMask[n];
    for (TGraph::EdgeId eid : graph.getNode(n)->getInEdges()) {
      const TEdge* edge = graph.getEdge(eid);
      if (edge != nullptr && !edge->isDisabled()) {
        atNode |= edge->getFromClocks();
      }
    }
    for (TGraph::EdgeId eid : graph.getNode(n)->getInEdges()) {
      TEdge* edge = graph.getEdge(eid);
      if (edge != nullptr && !edge->isDisabled()) {
        edge->setToClocks(atNode);
      }
    }
  }
}

void TClockPropagator::run(TGraph& graph) {
  propagateForward(graph);
  propagateBackward(graph);
}

}  // namespace naja::TG
