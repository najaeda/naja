// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLogicalCone.h"

#include <algorithm>
#include <deque>
#include <limits>
#include <map>
#include <set>
#include <utility>

#include "NLException.h"
#include "SNLBitTerm.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLEquipotential.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLNetComponent.h"
#include "SNLPath.h"

namespace {

using namespace naja::NL;

struct SNLLogicalConeExtractor {
  using Node = SNLLogicalCone::Node;
  using NodeID = SNLLogicalCone::NodeID;
  using NodeKind = SNLLogicalCone::NodeKind;
  using Direction = SNLLogicalCone::Direction;

  SNLLogicalConeExtractor(
    Direction direction,
    std::vector<Node>& nodes,
    NodeID& root,
    std::vector<NodeID>& leaves):
    direction_(direction),
    nodes_(nodes),
    root_(root),
    leaves_(leaves)
  {}

  void extract(const SNLOccurrence& start) {
    if (not start.isValid() or
        (not start.getBitTerm() and not start.getInstTerm())) {
      throw NLException(
        "SNLLogicalCone start must be a valid single-bit "
        "SNLNetComponent occurrence");
    }
    root_ = getOrCreate(start, NodeKind::Root);
    queue_.emplace_back(root_, start);

    while (not queue_.empty()) {
      auto [parentID, netComponentOccurrence] = queue_.front();
      queue_.pop_front();
      extractEquipotential(parentID, netComponentOccurrence);
    }
  }

  private:
    NodeID getOrCreate(
      const SNLOccurrence& occurrence,
      NodeKind kind) {
      auto found = indexOf_.find(occurrence);
      if (found != indexOf_.end()) {
        return found->second;
      }
      if (nodes_.size() >= std::numeric_limits<NodeID>::max()) {
        throw NLException("SNLLogicalCone node count exceeds NodeID capacity");
      }
      auto id = static_cast<NodeID>(nodes_.size());
      nodes_.push_back(Node {occurrence, kind, {}, {}});
      indexOf_.emplace(occurrence, id);
      if (isLeaf(kind)) {
        leaves_.push_back(id);
      }
      return id;
    }

    static bool isLeaf(NodeKind kind) {
      return kind == NodeKind::Flop or
        kind == NodeKind::Ports or
        kind == NodeKind::Blackbox;
    }

    bool wouldCreateCycle(NodeID parentID, NodeID childID) const {
      if (parentID == childID) {
        return true;
      }
      std::vector<NodeID> stack {childID};
      std::set<NodeID> visited;
      while (not stack.empty()) {
        auto current = stack.back();
        stack.pop_back();
        if (current == parentID) {
          return true;
        }
        if (not visited.insert(current).second) {
          continue;
        }
        const auto& next = nodes_[current].next;
        stack.insert(stack.end(), next.begin(), next.end());
      }
      return false;
    }

    void addEdge(NodeID parentID, NodeID childID) {
      auto& next = nodes_[parentID].next;
      if (std::find(next.begin(), next.end(), childID) != next.end()) {
        return;
      }
      if (wouldCreateCycle(parentID, childID)) {
        return;
      }
      next.push_back(childID);
      nodes_[childID].prev.push_back(parentID);
    }

    void extractEquipotential(
      NodeID parentID,
      const SNLOccurrence& netComponentOccurrence) {
      SNLEquipotential equipotential(netComponentOccurrence);
      const auto cellDirection =
        direction_ == Direction::FanIn ?
          SNLNetComponent::Direction::Output :
          SNLNetComponent::Direction::Input;
      const auto portDirection =
        direction_ == Direction::FanIn ?
          SNLNetComponent::Direction::Input :
          SNLNetComponent::Direction::Output;

      for (const auto& occurrence: equipotential.getInstTermOccurrences()) {
        auto instTerm = occurrence.getInstTerm();
        if (not instTerm or instTerm->getDirection() != cellDirection) {
          continue;
        }
        auto instance = instTerm->getInstance();
        auto model = instance->getModel();
        auto instanceOccurrence = SNLOccurrence(occurrence.getPath(), instance);

        NodeKind kind = NodeKind::Internal;
        if (SNLDesignModeling::isSequential(model)) {
          kind = NodeKind::Flop;
        } else if (not SNLDesignModeling::hasModeling(model)) {
          kind = NodeKind::Blackbox;
        }

        auto childID = getOrCreate(instanceOccurrence, kind);
        addEdge(parentID, childID);
        if (kind != NodeKind::Internal) {
          continue;
        }
        if (not expandedPins_.insert(occurrence).second) {
          continue;
        }

        auto crossed =
          direction_ == Direction::FanIn ?
            SNLDesignModeling::getCombinatorialInputs(instTerm) :
            SNLDesignModeling::getCombinatorialOutputs(instTerm);
        for (auto nextInstTerm: crossed) {
          queue_.emplace_back(
            childID,
            SNLOccurrence(occurrence.getPath(), nextInstTerm));
        }
      }

      for (auto term: equipotential.getTerms()) {
        if (term->getDirection() != portDirection) {
          continue;
        }
        auto portID = getOrCreate(SNLOccurrence(term), NodeKind::Ports);
        addEdge(parentID, portID);
      }
    }

    Direction direction_;
    std::vector<Node>& nodes_;
    NodeID& root_;
    std::vector<NodeID>& leaves_;
    std::map<SNLOccurrence, NodeID> indexOf_ {};
    std::set<SNLOccurrence> expandedPins_ {};
    std::deque<std::pair<NodeID, SNLOccurrence>> queue_ {};
};

}  // namespace

namespace naja::NL {

SNLLogicalCone::SNLLogicalCone(
  const SNLOccurrence& start,
  Direction direction):
  direction_(direction) {
  SNLLogicalConeExtractor extractor(
    direction_, nodes_, root_, leaves_);
  extractor.extract(start);
}

}  // namespace naja::NL
