// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>

namespace naja::TG {

class TGraph;

/// \brief detects cycles in a TGraph and disables one edge per cycle so
/// that traversals filtering out TEdge::isDisabled() edges (such as clock
/// domain propagation) see an acyclic graph.
///
/// Runs an iterative DFS over enabled edges; any edge reaching a node
/// already on the current DFS path (a "back edge") is disabled. This is
/// a standard heuristic feedback-arc-set: it breaks every cycle, but is
/// not guaranteed to disable the minimum number of edges, and which edge
/// gets cut per cycle depends on traversal/edge order.
class TLoopBreaker {
  public:
    TLoopBreaker() = delete;

    /// \return the number of edges disabled.
    static std::size_t run(TGraph& graph);
};

}  // namespace naja::TG
