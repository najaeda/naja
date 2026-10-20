// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <vector>

namespace naja::TG {

class TGraph;

/// Computes launch (FromClocks) and capture (ToClocks) masks on ordinary edges.
/// run() breaks loops, propagates clock seeds forward, then seeds capture
/// endpoints from capture-edge sources and propagates backward, stopping at
/// flop data outputs. Capture edges never participate in ordinary traversal.
/// Masks are recomputed on every run; disabled edges have empty masks.
/// An empty ToClocks means no capture endpoint was reached.
/// Latch analysis is unsupported and rejected explicitly.
class TClockPropagator {
  public:
    TClockPropagator() = delete;

    static void run(TGraph& graph);

  private:
    static std::vector<std::size_t> propagateForward(TGraph& graph);
    static void propagateBackward(TGraph& graph, const std::vector<std::size_t>& order);
};

}  // namespace naja::TG
