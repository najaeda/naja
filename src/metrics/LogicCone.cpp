// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "LogicCone.h"

#include <algorithm>
#include <deque>
#include <limits>
#include <map>
#include <set>
#include <utility>

#include "DNL.h"
#include "NLException.h"
#include "NLUniverse.h"
#include "SNLBitTerm.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLTerm.h"

namespace {

using namespace naja::DNL;
using namespace naja::NL;
using naja::NAJA_METRICS::LogicCone;

SNLDesign* inferTopDesign(const SNLOccurrence& occurrence) {
  if (not occurrence.isValid()) {
    return nullptr;
  }
  auto path = occurrence.getPath();
  if (not path.empty()) {
    return path.getDesign();
  }
  if (auto term = occurrence.getBitTerm()) {
    return term->getDesign();
  }
  if (auto instTerm = occurrence.getInstTerm()) {
    return instTerm->getInstance()->getDesign();
  }
  return nullptr;
}

SNLOccurrence getInstanceOccurrence(const DNLInstanceFull& instance) {
  return SNLOccurrence(
    instance.getPath().getHeadPath(),
    instance.getSNLInstance());
}

struct LogicConeExtractor {
  using Node = LogicCone::Node;
  using NodeID = LogicCone::NodeID;
  using NodeKind = LogicCone::NodeKind;
  using Direction = LogicCone::Direction;

  LogicConeExtractor(
    DNLFull& dnl,
    Direction direction,
    std::vector<Node>& nodes,
    NodeID& root,
    std::vector<NodeID>& leaves):
    dnl_(dnl),
    direction_(direction),
    nodes_(nodes),
    root_(root),
    leaves_(leaves)
  {}

  void extract(const SNLOccurrence& start) {
    if (not start.isValid() or
        (not start.getBitTerm() and not start.getInstTerm())) {
      throw NLException(
        "LogicCone start must be a valid single-bit "
        "SNLNetComponent occurrence");
    }

    auto startTerminal = findTerminal(start);
    if (not startTerminal) {
      throw NLException("LogicCone start is not present in the current DNL");
    }

    root_ = getOrCreate(start, NodeKind::Root);
    queue_.emplace_back(root_, startTerminal->getID());

    while (not queue_.empty()) {
      auto [parentID, terminalID] = queue_.front();
      queue_.pop_front();
      extractIso(parentID, terminalID);
    }
  }

  private:
    const DNLInstanceFull* findInstance(const SNLPath& path) const {
      auto instance = &dnl_.getTop();
      for (auto snlInstance: path.getInstances()) {
        auto& child = instance->getChildInstance(snlInstance);
        if (child.isNull()) {
          return nullptr;
        }
        instance = &child;
      }
      return instance;
    }

    const DNLTerminalFull* findTerminal(
      const SNLOccurrence& occurrence) const {
      if (auto bitTerm = occurrence.getBitTerm()) {
        auto instance = findInstance(occurrence.getPath());
        if (not instance) {
          return nullptr;
        }
        auto& terminal = instance->getTerminalFromBitTerm(bitTerm);
        return terminal.isNull() ? nullptr : &terminal;
      }

      auto instTerm = occurrence.getInstTerm();
      if (not instTerm) {
        return nullptr;
      }
      auto parent = findInstance(occurrence.getPath());
      if (not parent) {
        return nullptr;
      }
      auto& instance = parent->getChildInstance(instTerm->getInstance());
      if (instance.isNull()) {
        return nullptr;
      }
      auto& terminal = instance.getTerminal(instTerm);
      return terminal.isNull() ? nullptr : &terminal;
    }

    NodeID getOrCreate(
      const SNLOccurrence& occurrence,
      NodeKind kind) {
      auto found = indexOf_.find(occurrence);
      if (found != indexOf_.end()) {
        return found->second;
      }
      // LCOV_EXCL_START
      if (nodes_.size() >= std::numeric_limits<NodeID>::max()) {
        throw NLException("LogicCone node count exceeds NodeID capacity");
      }
      // LCOV_EXCL_STOP
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

    NodeKind getNodeKind(const DNLInstanceFull& instance) const {
      auto model = instance.getSNLInstance()->getModel();
      if (SNLDesignModeling::isSequential(model)) {
        return NodeKind::Flop;
      }
      if (not SNLDesignModeling::hasModeling(model)) {
        return NodeKind::Blackbox;
      }
      return NodeKind::Internal;
    }

    void expandThroughInstance(
      NodeID childID,
      const DNLTerminalFull& terminal,
      NodeKind kind) {
      if (kind != NodeKind::Internal) {
        return;
      }
      if (not expandedTerms_.insert(terminal.getID()).second) {
        return;
      }

      auto crossed =
        direction_ == Direction::FanIn ?
          SNLDesignModeling::getCombinatorialInputs(terminal.getSnlTerm()) :
          SNLDesignModeling::getCombinatorialOutputs(terminal.getSnlTerm());
      auto& instance = terminal.getDNLInstance();
      for (auto nextInstTerm: crossed) {
        auto& nextTerminal = instance.getTerminal(nextInstTerm);
        if (nextTerminal.isNull()) {
          continue;
        }
        queue_.emplace_back(childID, nextTerminal.getID());
      }
    }

    void addTerminal(NodeID parentID, const DNLTerminalFull& terminal) {
      if (terminal.isTopPort()) {
        auto portID = getOrCreate(terminal.getOccurrence(), NodeKind::Ports);
        addEdge(parentID, portID);
        return;
      }

      auto& instance = terminal.getDNLInstance();
      auto kind = getNodeKind(instance);
      auto childID = getOrCreate(getInstanceOccurrence(instance), kind);
      addEdge(parentID, childID);
      expandThroughInstance(childID, terminal, kind);
    }

    void extractIso(NodeID parentID, DNLID terminalID) {
      auto isoID = dnl_.getIsoIdfromTermId(terminalID);
      if (isoID == DNLID_MAX) {
        return;
      }
      const auto& iso = dnl_.getDNLIsoDB().getIsoFromIsoIDconst(isoID);
      const auto& terminals =
        direction_ == Direction::FanIn ? iso.getDrivers() : iso.getReaders();
      for (auto nextTerminalID: terminals) {
        const auto& terminal = dnl_.getDNLTerminalFromID(nextTerminalID);
        if (terminal.isNull()) {
          continue;
        }
        addTerminal(parentID, terminal);
      }
    }

    DNLFull& dnl_;
    Direction direction_;
    std::vector<Node>& nodes_;
    NodeID& root_;
    std::vector<NodeID>& leaves_;
    std::map<SNLOccurrence, NodeID> indexOf_ {};
    std::set<DNLID> expandedTerms_ {};
    std::deque<std::pair<NodeID, DNLID>> queue_ {};
};

}  // namespace

namespace naja::NAJA_METRICS {

LogicCone::LogicCone(
  const naja::NL::SNLOccurrence& start,
  Direction direction):
  direction_(direction) {
  auto top = inferTopDesign(start);
  if (not top or not naja::NL::NLUniverse::get()) {
    throw naja::NL::NLException(
      "LogicCone requires an occurrence in an existing NLUniverse");
  }
  if (naja::NL::NLUniverse::get()->getTopDesign() != top) {
    naja::NL::NLUniverse::get()->setTopDesign(top);
  }
  auto dnl = naja::DNL::get();
  LogicConeExtractor extractor(*dnl, direction_, nodes_, root_, leaves_);
  extractor.extract(start);
}

}  // namespace naja::NAJA_METRICS
