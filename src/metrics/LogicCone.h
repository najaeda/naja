// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "SNLLogicalCone.h"

namespace naja::NAJA_METRICS {

class LogicCone { // LCOV_EXCL_LINE declaration attributed by gcov
  public:
    using Direction = naja::NL::SNLLogicalCone::Direction;
    using NodeID = naja::NL::SNLLogicalCone::NodeID;
    using NodeKind = naja::NL::SNLLogicalCone::NodeKind;
    using Node = naja::NL::SNLLogicalCone::Node;

    LogicCone(const naja::NL::SNLOccurrence& start, Direction direction);

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

}  // namespace naja::NAJA_METRICS
