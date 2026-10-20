// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>

namespace naja::TG {

class TGraph;

/// Associates a clock/gate node with a capture endpoint (C -> D).
/// Stored separately from signal edges; never traversed by ordinary graph passes.
/// The builder is responsible for selecting compatible endpoint roles.
class TCaptureEdge {
  public:
    using NodeId = std::size_t;
    using CaptureEdgeId = std::size_t;

    CaptureEdgeId getIndex() const { return index_; }
    NodeId getSource() const { return source_; }
    NodeId getTarget() const { return target_; }
    bool isAlive() const { return alive_; }

  private:
    friend class TGraph;

    TCaptureEdge(CaptureEdgeId index, NodeId source, NodeId target)
      : index_(index), source_(source), target_(target) {}

    void reset(CaptureEdgeId index, NodeId source, NodeId target) {
      index_ = index;
      source_ = source;
      target_ = target;
      alive_ = true;
    }
    void deactivate() { alive_ = false; }

    CaptureEdgeId index_;
    NodeId source_;
    NodeId target_;
    bool alive_ {true};
};

}  // namespace naja::TG
