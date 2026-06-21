// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <vector>

#include "SNLOccurrence.h"

namespace naja::NL {

class SNLLogicalCone { // LCOV_EXCL_LINE declaration attributed by gcov
  public:
    enum class Direction { FanIn, FanOut };

    using NodeID = uint32_t;

    enum class NodeKind { Root, Internal, Flop, Ports, Blackbox };

    struct Node { // LCOV_EXCL_LINE declaration attributed by gcov
      SNLOccurrence       occurrence {};
      NodeKind            kind       {NodeKind::Internal};
      std::vector<NodeID> next       {};
      std::vector<NodeID> prev       {};
    };

    SNLLogicalCone(const SNLOccurrence& start, Direction direction);

    Direction getDirection() const { return direction_; }

    const std::vector<Node>& getNodes() const { return nodes_; }
    NodeID getRoot() const { return root_; }
    const std::vector<NodeID>& getLeaves() const { return leaves_; }

    size_t getNodeCount() const { return nodes_.size(); }

  private:
    Direction direction_;
    std::vector<Node> nodes_ {};
    NodeID root_ {0};
    std::vector<NodeID> leaves_ {};
};

}  // namespace naja::NL
