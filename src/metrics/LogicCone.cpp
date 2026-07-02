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

using Node = LogicCone::Node;
using NodeID = LogicCone::NodeID;
using NodeKind = LogicCone::NodeKind;
using Direction = LogicCone::Direction;

SNLDesign* inferTopDesign(const SNLOccurrence& occurrence) {
  if (not occurrence.isValid()) {
    return nullptr;
  }
  auto path = occurrence.getPath();
  if (not path.empty()) {
    return path.getDesign();
  }
  if (auto term = dynamic_cast<SNLTerm*>(occurrence.getObject())) {
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

struct ConeData {
  std::vector<Node> nodes {};
  NodeID root {0};
  std::vector<NodeID> leaves {};
  std::map<SNLOccurrence, NodeID> indexOf {};
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
    const SNLOccurrence& occurrence,
    NodeKind kind) {
    auto found = indexOf.find(occurrence);
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
    nodes.push_back(Node {occurrence, kind, {}, {}});
    indexOf.emplace(occurrence, id);
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
    ConeData& data):
    dnl_(dnl),
    direction_(direction),
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

    auto root = data_.getOrCreate(start, NodeKind::Root);
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
    const DNLInstanceFull* findInstance(const SNLPath& path) const {
      auto instance = &dnl_.getTop();
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
      if (terminal.isTopPort()) {
        auto portID =
          data_.getOrCreate(terminal.getOccurrence(), NodeKind::Ports);
        data_.addEdge(parentID, portID);
        return;
      }

      auto& instance = terminal.getDNLInstance();
      auto kind = getNodeKind(instance);
      auto childID = data_.getOrCreate(getInstanceOccurrence(instance), kind);
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
      for (auto nextTerminalID: terminals) {
        const auto& terminal = dnl_.getDNLTerminalFromID(nextTerminalID);
        // LCOV_EXCL_START
        if (terminal.isNull()) {
          continue;
        }
        // LCOV_EXCL_STOP
        addTerminal(parentID, terminal);
      }
    }

    DNLFull& dnl_;
    Direction direction_;
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
        local.nodes[i].occurrence,
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

ConeData extractSingleBitCone(
    DNLFull& dnl,
    Direction direction,
    const SNLOccurrence& start) {
  ConeData data;
  LogicConeExtractor extractor(dnl, direction, data);
  extractor.extract(start);
  return data;
}

ConeData extractBusCone(
    DNLFull& dnl,
    Direction direction,
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
    LogicConeExtractor extractor(dnl, direction, local);
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
  merged.root = merged.getOrCreate(busOccurrence, NodeKind::Root);
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
  auto top = inferTopDesign(start);
  if (not top or not naja::NL::NLUniverse::get()) {
    throw naja::NL::NLException(
      "LogicCone requires an occurrence in an existing NLUniverse");
  }
  if (naja::NL::NLUniverse::get()->getTopDesign() != top) {
    naja::NL::NLUniverse::get()->setTopDesign(top);
  }
  auto dnl = naja::DNL::get();
  auto data = [&]() {
    if (auto bus = dynamic_cast<naja::NL::SNLBusTerm*>(start.getObject())) {
      return extractBusCone(*dnl, direction_, start, bus);
    }
    return extractSingleBitCone(*dnl, direction_, start);
  }();
  nodes_ = std::move(data.nodes);
  root_ = data.root;
  leaves_ = std::move(data.leaves);
}

}  // namespace naja::NAJA_METRICS
