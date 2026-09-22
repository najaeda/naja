// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TLoopBreaker.h"

#include <utility>
#include <vector>

#include "TEdge.h"
#include "TGraph.h"
#include "TNode.h"

namespace naja::TG {

namespace {

enum class Color { White, Gray, Black };

}  // namespace

std::size_t TLoopBreaker::run(TGraph& graph) {
  std::size_t disabledCount = 0;
  const std::size_t nodesCount = graph.getNodesStorage().size();
  std::vector<Color> color(nodesCount, Color::White);

  //stack entries are (node, index of the next out-edge of that node to visit).
  std::vector<std::pair<TGraph::NodeId, std::size_t>> stack;

  for (TGraph::NodeId start = 0; start < nodesCount; ++start) {
    if (!graph.isNodeValid(start) || color[start] != Color::White) {
      continue;
    }

    color[start] = Color::Gray;
    stack.emplace_back(start, 0);

    while (!stack.empty()) {
      const TGraph::NodeId cur = stack.back().first;
      const std::size_t edgeIdx = stack.back().second;
      const auto& outEdges = graph.getNode(cur)->getOutEdges();

      if (edgeIdx >= outEdges.size()) {
        color[cur] = Color::Black;
        stack.pop_back();
        continue;
      }

      //Advance this frame's edge cursor before any push_back below, since
      //that can reallocate `stack` and invalidate this reference.
      stack.back().second = edgeIdx + 1;

      TEdge* edge = graph.getEdge(outEdges[edgeIdx]);
      if (edge == nullptr || edge->isDisabled()) {
        continue;
      }

      const TGraph::NodeId next = edge->getTarget();
      switch (color[next]) {
        case Color::White:
          color[next] = Color::Gray;
          stack.emplace_back(next, 0);
          break;
        case Color::Gray:
          //`next` is an ancestor on the current DFS path: this edge closes
          //a cycle. Disable it rather than following it.
          edge->setDisabled(true);
          ++disabledCount;
          break;
        case Color::Black:
          //Forward/cross edge to an already fully-explored node: not part
          //of a cycle through the current path.
          break;
      }
    }
  }

  return disabledCount;
}

}  // namespace naja::TG
