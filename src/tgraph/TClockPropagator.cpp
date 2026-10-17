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
#include "TLoopBreaker.h"
#include "TNode.h"

namespace naja::TG {

std::vector<std::size_t> TClockPropagator::propagateForward(TGraph& graph) {
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
  return queue;
}

void TClockPropagator::propagateBackward(TGraph& graph, const std::vector<std::size_t>& order) {
  const std::size_t nodesCount = graph.getNodesStorage().size();
  const std::size_t clocksCount = graph.getClocksCount();

  //Direct clock seeds must be included even when C has no incoming edges.
  std::vector<TEdge::ClockMask> seedMask(nodesCount, TEdge::ClockMask(clocksCount));
  for (std::size_t c = 0; c < clocksCount; ++c) {
    const TClock* clock = graph.getClock(c);
    seedMask[clock->getNode()].set(c);
  }

  std::vector<TEdge::ClockMask> capture(nodesCount, TEdge::ClockMask(clocksCount));
  for (const auto& edge : graph.getCaptureEdgesStorage()) {
    if (!edge.isAlive()) {
      continue;
    }
    const auto source = edge.getSource();
    TEdge::ClockMask atClock = seedMask[source];
    for (auto eid : graph.getNode(source)->getInEdges()) {
      const auto* incoming = graph.getEdge(eid);
      if (incoming != nullptr && !incoming->isDisabled()) {
        atClock |= incoming->getFromClocks();
      }
    }
    capture[edge.getTarget()] |= atClock;
  }

  //Reverse topological order visits every capture consumer before its drivers.
  for (auto it = order.rbegin(); it != order.rend(); ++it) {
    const auto n = *it;
    if (graph.getNode(n)->isFlopDataOutput()) {
      continue;
    }
    for (TGraph::EdgeId eid : graph.getNode(n)->getInEdges()) {
      TEdge* edge = graph.getEdge(eid);
      if (edge != nullptr && !edge->isDisabled()) {
        edge->setToClocks(capture[n]);
        capture[edge->getSource()] |= capture[n];
      }
    }
  }
}

void TClockPropagator::run(TGraph& graph) {
  const TEdge::ClockMask empty(graph.getClocksCount());
  for (const auto& edge : graph.getEdgesStorage()) {
    if (edge.isAlive()) {
      graph.getEdge(edge.getIndex())->setFromClocks(empty);
      graph.getEdge(edge.getIndex())->setToClocks(empty);
    }
  }
  for (const auto& node : graph.getNodesStorage()) {
    if (node.isAlive() && node.isLatch()) {
      throw std::logic_error("TClockPropagator::run: latch analysis is unsupported");
    }
  }
  for (const auto& clock : graph.getClocksStorage()) {
    if (!graph.isNodeValid(clock.getNode())) {
      throw std::logic_error("TClockPropagator::run: clock seed node was removed");
    }
  }
  TLoopBreaker::run(graph);
  const auto order = propagateForward(graph);
  propagateBackward(graph, order);
}

}  // namespace naja::TG
