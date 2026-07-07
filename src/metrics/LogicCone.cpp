// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "LogicCone.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <limits>
#include <map>
#include <set>
#include <utility>

#include <tbb/blocked_range.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>

#include "DNL.h"
#include "NLException.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBitTerm.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLTerm.h"
#include "SNLUtils.h"

namespace {

using namespace naja::DNL;
using namespace naja::NL;
using naja::NAJA_METRICS::LogicCone;

using PublicNode = LogicCone::Node;
using NodeID = LogicCone::NodeID;
using NodeKind = LogicCone::NodeKind;
using Direction = LogicCone::Direction;

bool designContains(const SNLDesign* root, const SNLDesign* target) {
  if (not root or not target) {
    return false;
  }
  std::vector<const SNLDesign*> stack {root};
  std::set<const SNLDesign*> visited;
  while (not stack.empty()) {
    auto design = stack.back();
    stack.pop_back();
    if (design == target) {
      return true;
    }
    if (not visited.insert(design).second) {
      continue;
    }
    for (auto instance : design->getInstances()) {
      stack.push_back(instance->getModel());
    }
  }
  return false;
}

DNLID getLocalTopID(
    DNLFull& dnl,
    const SNLOccurrence& occurrence) {
  auto localTopID = dnl.getDNLInstanceIDForDesign(occurrence.getDesign());
  // LCOV_EXCL_START
  if (localTopID == DNLID_MAX) {
    throw NLException("LogicCone start design is not present in the current DNL");
  }
  // LCOV_EXCL_STOP
  return localTopID;
}

enum class NodeKeyKind { BusRoot, Instance, Terminal };

struct NodeKey {
  NodeKeyKind kind {NodeKeyKind::Terminal};
  DNLID id {DNLID_MAX};

  bool operator<(const NodeKey& rhs) const {
    if (kind != rhs.kind) {
      return kind < rhs.kind;
    }
    return id < rhs.id;
  }
};

struct Node {
  NodeKey key {};
  NodeKind kind {NodeKind::Internal};
  std::vector<NodeID> next {};
  std::vector<NodeID> prev {};
};

struct ConeData {
  std::vector<Node> nodes {};
  NodeID root {0};
  std::vector<NodeID> leaves {};
  std::map<NodeKey, NodeID> indexOf {};
  std::set<DNLID> expandedTerms {};
  std::deque<std::pair<NodeID, DNLID>> queue {};
  std::vector<NodeID> bitRoots {};

  static bool isLeaf(NodeKind kind) {
    return kind == NodeKind::Flop or
      kind == NodeKind::Ports or
      kind == NodeKind::Blackbox;
  }

  void eraseLeaf(NodeID id) {
    leaves.erase(std::remove(leaves.begin(), leaves.end(), id), leaves.end());
  }

  NodeID getOrCreate(
    NodeKey key,
    NodeKind kind) {
    auto found = indexOf.find(key);
    if (found != indexOf.end()) {
      auto id = found->second;
      if (kind == NodeKind::Root and nodes[id].kind != NodeKind::Root) {
        if (isLeaf(nodes[id].kind)) {
          eraseLeaf(id);
        }
        nodes[id].kind = NodeKind::Root;
      }
      return id;
    }

    // LCOV_EXCL_START
    if (nodes.size() >= std::numeric_limits<NodeID>::max()) {
      throw NLException("LogicCone node count exceeds NodeID capacity");
    }
    // LCOV_EXCL_STOP
    auto id = static_cast<NodeID>(nodes.size());
    nodes.push_back(Node {key, kind, {}, {}});
    indexOf.emplace(key, id);
    if (isLeaf(kind)) {
      leaves.push_back(id);
    }
    return id;
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
        continue; // LCOV_EXCL_LINE defensive guard for repeated graph visits
      }
      const auto& next = nodes[current].next;
      stack.insert(stack.end(), next.begin(), next.end());
    }
    return false;
  }

  void addEdge(NodeID parentID, NodeID childID) {
    auto& next = nodes[parentID].next;
    if (std::find(next.begin(), next.end(), childID) != next.end()) {
      return;
    }
    if (wouldCreateCycle(parentID, childID)) {
      return;
    }
    next.push_back(childID);
    nodes[childID].prev.push_back(parentID);
  }
};

struct LogicConeExtractor {
  LogicConeExtractor(
    DNLFull& dnl,
    Direction direction,
    DNLID localTopID,
    ConeData& data):
    dnl_(dnl),
    direction_(direction),
    localTopID_(localTopID),
    data_(data)
  {}

  NodeID extract(const SNLOccurrence& start) {
    // LCOV_EXCL_START
    if (not start.isValid() or
        (not start.getBitTerm() and not start.getInstTerm())) {
      throw NLException(
        "LogicCone start must be a valid single-bit "
        "SNLNetComponent occurrence");
    }
    // LCOV_EXCL_STOP

    auto startTerminal = findTerminal(start);
    // LCOV_EXCL_START
    if (not startTerminal) {
      throw NLException("LogicCone start is not present in the current DNL");
    }
    // LCOV_EXCL_STOP

    auto root = data_.getOrCreate(
        {NodeKeyKind::Terminal, startTerminal->getID()},
        NodeKind::Root);
    data_.root = root;
    data_.queue.clear();
    data_.queue.emplace_back(root, startTerminal->getID());

    while (not data_.queue.empty()) {
      auto [parentID, terminalID] = data_.queue.front();
      data_.queue.pop_front();
      extractIso(parentID, terminalID);
    }
    return root;
  }

  private:
    const DNLInstanceFull& localTop() const {
      return dnl_.getDNLInstanceFromID(localTopID_);
    }

    bool isUnderLocalTop(const DNLInstanceFull& instance) const {
      return instance.isUnder(localTop());
    }

    bool isLocalPort(const DNLTerminalFull& terminal) const {
      if (localTopID_ == dnl_.getTop().getID()) {
        return terminal.isTopPort();
      }
      return terminal.getDNLInstance().getID() == localTopID_;
    }

    bool isBoundaryPortForDirection(const DNLTerminalFull& terminal) const {
      if (not isLocalPort(terminal)) {
        return false;
      }
      const auto portDirection =
        direction_ == Direction::FanIn ?
          SNLTerm::Direction::Input :
          SNLTerm::Direction::Output;
      return terminal.getSnlBitTerm()->getDirection() == portDirection;
    }

    static void pushTerminal(std::vector<DNLID>& stack, DNLID terminalID) {
      if (terminalID != DNLID_MAX) {
        stack.push_back(terminalID);
      }
    }

    std::vector<DNLID> collectLocalBoundaryPorts(DNLID seedTerminalID) const {
      std::vector<DNLID> ports;
      if (localTopID_ == dnl_.getTop().getID()) {
        return ports;
      }

      std::vector<DNLID> stack {seedTerminalID};
      std::set<DNLID> visited;
      while (not stack.empty()) {
        auto terminalID = stack.back();
        stack.pop_back();
        if (not visited.insert(terminalID).second) {
          continue; // LCOV_EXCL_LINE repeated terminal graph visit
        }

        const auto& terminal = dnl_.getDNLTerminalFromID(terminalID);
        // LCOV_EXCL_START
        if (terminal.isNull()) {
          continue;
        }
        // LCOV_EXCL_STOP
        if (isLocalPort(terminal)) {
          if (isBoundaryPortForDirection(terminal)) {
            ports.push_back(terminalID);
          }
          continue;
        }

        auto pushNetTerms = [&](const DNLInstanceFull& context, auto net) {
          for (auto instTerm : net->getInstTerms()) {
            const auto& child =
                context.getChildInstance(instTerm->getInstance());
            // LCOV_EXCL_START
            if (child.isNull()) {
              continue;
            }
            // LCOV_EXCL_STOP
            pushTerminal(stack, child.getTerminal(instTerm).getID());
          }
          for (auto bitTerm : net->getBitTerms()) {
            pushTerminal(
                stack,
                context.getTerminalFromBitTerm(bitTerm).getID());
          }
        };

        auto bitTermNet = terminal.getSnlBitTerm()->getNet();
        if (bitTermNet and
            not terminal.getDNLInstance().getSNLModel()->isAssign()) {
          pushNetTerms(terminal.getDNLInstance(), bitTermNet);
        }

        auto instTerm = terminal.getSnlTerm();
        if (not terminal.getDNLInstance().isTop() and
            instTerm and instTerm->getNet()) {
          pushNetTerms(
              terminal.getDNLInstance().getParentInstance(),
              instTerm->getNet());
        }
      }
      return ports;
    }

    const DNLInstanceFull* findInstance(const SNLPath& path) const {
      auto instance = &localTop();
      for (auto snlInstance: path.getInstances()) {
        auto& child = instance->getChildInstance(snlInstance);
        // LCOV_EXCL_START
        if (child.isNull()) {
          return nullptr;
        }
        // LCOV_EXCL_STOP
        instance = &child;
      }
      return instance;
    }

    const DNLTerminalFull* findTerminal(
      const SNLOccurrence& occurrence) const {
      if (auto bitTerm = occurrence.getBitTerm()) {
        auto instance = findInstance(occurrence.getPath());
        // LCOV_EXCL_START
        if (not instance) {
          return nullptr;
        }
        // LCOV_EXCL_STOP
        auto& terminal = instance->getTerminalFromBitTerm(bitTerm);
        // LCOV_EXCL_START
        if (terminal.isNull()) {
          return nullptr;
        }
        // LCOV_EXCL_STOP
        // LCOV_EXCL_START
        if (not isUnderLocalTop(terminal.getDNLInstance())) {
          return nullptr;
        }
        // LCOV_EXCL_STOP
        return &terminal;
      }

      auto instTerm = occurrence.getInstTerm();
      // LCOV_EXCL_START
      if (not instTerm) {
        return nullptr;
      }
      // LCOV_EXCL_STOP
      auto parent = findInstance(occurrence.getPath());
      // LCOV_EXCL_START
      if (not parent) {
        return nullptr;
      }
      // LCOV_EXCL_STOP
      auto& instance = parent->getChildInstance(instTerm->getInstance());
      // LCOV_EXCL_START
      if (instance.isNull()) {
        return nullptr;
      }
      // LCOV_EXCL_STOP
      auto& terminal = instance.getTerminal(instTerm);
      // LCOV_EXCL_START
      if (terminal.isNull()) {
        return nullptr;
      }
      // LCOV_EXCL_STOP
      // LCOV_EXCL_START
      if (not isUnderLocalTop(terminal.getDNLInstance())) {
        return nullptr;
      }
      // LCOV_EXCL_STOP
      return &terminal;
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
      if (not data_.expandedTerms.insert(terminal.getID()).second) {
        return;
      }

      auto crossed =
        direction_ == Direction::FanIn ?
          SNLDesignModeling::getCombinatorialInputs(terminal.getSnlTerm()) :
          SNLDesignModeling::getCombinatorialOutputs(terminal.getSnlTerm());
      auto& instance = terminal.getDNLInstance();
      for (auto nextInstTerm: crossed) {
        auto& nextTerminal = instance.getTerminal(nextInstTerm);
        // LCOV_EXCL_START
        if (nextTerminal.isNull()) {
          continue;
        }
        // LCOV_EXCL_STOP
        data_.queue.emplace_back(childID, nextTerminal.getID());
      }
    }

    void addTerminal(NodeID parentID, const DNLTerminalFull& terminal) {
      if (isLocalPort(terminal)) {
        auto portID = data_.getOrCreate(
            {NodeKeyKind::Terminal, terminal.getID()},
            NodeKind::Ports);
        data_.addEdge(parentID, portID);
        return;
      }

      auto& instance = terminal.getDNLInstance();
      auto kind = getNodeKind(instance);
      auto childID = data_.getOrCreate(
          {NodeKeyKind::Instance, instance.getID()},
          kind);
      data_.addEdge(parentID, childID);
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
      auto addTerminals = [&](const auto& terminalIDs, bool localBoundaryOnly) {
        for (auto nextTerminalID: terminalIDs) {
          const auto& terminal = dnl_.getDNLTerminalFromID(nextTerminalID);
          // LCOV_EXCL_START
          if (terminal.isNull()) {
            continue;
          }
          // LCOV_EXCL_STOP
          if (not isUnderLocalTop(terminal.getDNLInstance())) {
            continue;
          }
          if (localBoundaryOnly and not isBoundaryPortForDirection(terminal)) {
            continue;
          }
          addTerminal(parentID, terminal);
        }
      };
      addTerminals(terminals, false);

      const auto& localBoundaryTerminals =
        direction_ == Direction::FanIn ? iso.getReaders() : iso.getDrivers();
      addTerminals(localBoundaryTerminals, true);
      for (auto boundaryTerminalID : collectLocalBoundaryPorts(terminalID)) {
        addTerminal(parentID, dnl_.getDNLTerminalFromID(boundaryTerminalID));
      }
    }

    DNLFull& dnl_;
    Direction direction_;
    DNLID localTopID_;
    ConeData& data_;
};

std::vector<SNLOccurrence> getBusBitOccurrences(
    const SNLOccurrence& busOccurrence,
    SNLBusTerm* bus) {
  std::vector<SNLOccurrence> bits;
  bits.reserve(static_cast<size_t>(bus->getWidth()));
  auto path = busOccurrence.getPath();
  for (auto bit : bus->getBits()) {
    bits.emplace_back(path, bit);
  }
  return bits;
}

void mergeConeData(
    ConeData& merged,
    const ConeData& local,
    NodeID busRoot) {
  // LCOV_EXCL_START
  if (local.nodes.empty()) {
    return;
  }
  // LCOV_EXCL_STOP

  std::vector<NodeID> remap(local.nodes.size());
  for (size_t i = 0; i < local.nodes.size(); ++i) {
    remap[i] = merged.getOrCreate(
        local.nodes[i].key,
        local.nodes[i].kind);
  }
  for (size_t i = 0; i < local.nodes.size(); ++i) {
    for (auto next : local.nodes[i].next) {
      merged.addEdge(remap[i], remap[next]);
    }
  }
  for (auto bitRoot : local.bitRoots) {
    merged.addEdge(busRoot, remap[bitRoot]);
  }
}

SNLPath getRelativePath(DNLFull& dnl, DNLID localTopID, DNLID targetID) {
  SNLPath path;
  if (targetID == localTopID or targetID == DNLID_MAX) {
    return path;
  }

  std::vector<SNLInstance*> instances;
  auto currentID = targetID;
  while (currentID != localTopID) {
    const auto& current = dnl.getDNLInstanceFromID(currentID);
    // LCOV_EXCL_START
    if (current.isNull() or current.isTop()) {
      return SNLPath();
    }
    // LCOV_EXCL_STOP
    instances.push_back(current.getSNLInstance());
    currentID = current.getParentID();
  }

  std::reverse(instances.begin(), instances.end());
  for (auto instance : instances) {
    path = SNLPath(path, instance);
  }
  return path;
}

SNLOccurrence resolveOccurrence(
    DNLFull& dnl,
    const Node& node,
    const SNLOccurrence& startOccurrence,
    DNLID localTopID,
    bool hasBusRoot) {
  if (node.key.kind == NodeKeyKind::BusRoot) {
    return startOccurrence;
  }
  if (not hasBusRoot and node.kind == NodeKind::Root) {
    return startOccurrence;
  }
  if (node.key.kind == NodeKeyKind::Instance) {
    const auto& instance = dnl.getDNLInstanceFromID(node.key.id);
    // LCOV_EXCL_START
    if (instance.isNull() or instance.isTop()) {
      return SNLOccurrence();
    }
    // LCOV_EXCL_STOP
    return SNLOccurrence(
        getRelativePath(dnl, localTopID, instance.getParentID()),
        instance.getSNLInstance());
  }
  if (node.key.kind == NodeKeyKind::Terminal) {
    const auto& terminal = dnl.getDNLTerminalFromID(node.key.id);
    // LCOV_EXCL_START
    if (terminal.isNull()) {
      return SNLOccurrence();
    }
    // LCOV_EXCL_STOP
    return SNLOccurrence(
        getRelativePath(dnl, localTopID, terminal.getDNLInstance().getID()),
        terminal.getSnlBitTerm());
  }
  // LCOV_EXCL_START
  return SNLOccurrence();
  // LCOV_EXCL_STOP
}

std::vector<PublicNode> buildPublicNodes(
    DNLFull& dnl,
    const ConeData& data,
    const SNLOccurrence& startOccurrence,
    DNLID localTopID) {
  std::vector<PublicNode> nodes;
  nodes.reserve(data.nodes.size());
  auto hasBusRoot =
      data.root < data.nodes.size() and
      data.nodes[data.root].key.kind == NodeKeyKind::BusRoot;
  for (const auto& node : data.nodes) {
    nodes.push_back(PublicNode {
        resolveOccurrence(
            dnl, node, startOccurrence, localTopID, hasBusRoot),
        node.kind,
        node.next,
        node.prev});
  }
  return nodes;
}

ConeData extractSingleBitCone(
    DNLFull& dnl,
    Direction direction,
    DNLID localTopID,
    const SNLOccurrence& start) {
  ConeData data;
  LogicConeExtractor extractor(dnl, direction, localTopID, data);
  extractor.extract(start);
  return data;
}

ConeData extractBusCone(
    DNLFull& dnl,
    Direction direction,
    DNLID localTopID,
    const SNLOccurrence& busOccurrence,
    SNLBusTerm* bus) {
  auto bitOccurrences = getBusBitOccurrences(busOccurrence, bus);
  // LCOV_EXCL_START
  if (bitOccurrences.empty()) {
    throw NLException("LogicCone bus start has no bits");
  }
  // LCOV_EXCL_STOP

  SNLUtils::prepareForConcurrentAccess(dnl.getTopDesign());
  tbb::enumerable_thread_specific<ConeData> localCones;

  auto extractRange = [&](size_t begin, size_t end) {
    auto& local = localCones.local();
    LogicConeExtractor extractor(dnl, direction, localTopID, local);
    for (size_t i = begin; i < end; ++i) {
      local.bitRoots.push_back(extractor.extract(bitOccurrences[i]));
    }
  };

  if (not std::getenv("NON_MT")) {
    tbb::task_arena arena(tbb::task_arena::automatic);
    arena.execute([&]() {
      tbb::parallel_for(
          tbb::blocked_range<size_t>(0, bitOccurrences.size()),
          [&](const tbb::blocked_range<size_t>& range) {
            extractRange(range.begin(), range.end());
          });
    });
  } else {
    extractRange(0, bitOccurrences.size());
  }

  ConeData merged;
  merged.root = merged.getOrCreate(
      {NodeKeyKind::BusRoot, DNLID_MAX},
      NodeKind::Root);
  for (const auto& local : localCones) {
    mergeConeData(merged, local, merged.root);
  }
  return merged;
}

}  // namespace

namespace naja::NAJA_METRICS {

LogicCone::LogicCone(
  const naja::NL::SNLOccurrence& start,
  Direction direction):
  direction_(direction) {
  auto top = start.getDesign();
  if (not top or not naja::NL::NLUniverse::get()) {
    throw naja::NL::NLException(
      "LogicCone requires an occurrence in an existing NLUniverse");
  }
  auto universe = naja::NL::NLUniverse::get();
  auto currentTop = universe->getTopDesign();
  if (currentTop != top and not designContains(currentTop, top)) {
    universe->setTopDesign(top);
  }
  auto dnl = naja::DNL::get();
  auto localTopID = getLocalTopID(*dnl, start);
  auto data = [&]() {
    if (auto bus = dynamic_cast<naja::NL::SNLBusTerm*>(start.getObject())) {
      return extractBusCone(*dnl, direction_, localTopID, start, bus);
    }
    return extractSingleBitCone(*dnl, direction_, localTopID, start);
  }();
  nodes_ = buildPublicNodes(*dnl, data, start, localTopID);
  root_ = data.root;
  leaves_ = std::move(data.leaves);
}

}  // namespace naja::NAJA_METRICS
