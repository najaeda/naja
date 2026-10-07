// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace naja::TG {

class TGraph;

/// \brief propagates TClock seeds through a TGraph to fill every enabled
/// edge's FromClocks/ToClocks masks.
///
/// \pre the graph has no cycles among enabled edges - run TLoopBreaker
/// first. propagateForward() detects a residual cycle and throws rather
/// than silently leaving some nodes' masks incomplete.
///
/// Definitions, for a node n: resolved(n) is the union of every clock
/// seeded at n (TClock::getNode() == n) with resolved(p) for every node p
/// with an enabled edge p -> n. This is exactly the set of clocks that can
/// reach n by following enabled edges forward from their seed.
///
/// - propagateForward() computes resolved(n) for every node via a forward
///   topological walk, and sets FromClocks(edge A->B) = resolved(A): the
///   clocks reaching this edge from its source side.
/// - propagateBackward() must run after propagateForward(); it does not
///   walk the graph again. For every node B, it ORs together the
///   FromClocks already computed on all of B's enabled in-edges - i.e.
///   resolved(B), the full set of clocks arriving at B from every path,
///   not just one - and sets that as ToClocks on each of those in-edges.
///
/// hasClockCrossing() is then true on edge A->B exactly when some other
/// edge into B carries a clock that A's path does not: B is a point where
/// distinct clock domains converge.
class TClockPropagator {
  public:
    TClockPropagator() = delete;

    static void propagateForward(TGraph& graph);
    static void propagateBackward(TGraph& graph);

    /// \brief propagateForward() then propagateBackward().
    static void run(TGraph& graph);
};

}  // namespace naja::TG
