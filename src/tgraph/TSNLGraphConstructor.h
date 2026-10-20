// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <memory>

#include "SNLOccurrence.h"
#include "TGraph.h"
#include "TNodeProxy.h"
#include "TEdgeProxy.h"

namespace naja::NL {
class SNLDesign;
}

namespace naja::TG {

/// Terminal identity includes the containing hierarchy, even for shared models.
/// SNL objects and paths must outlive the graph and remain unchanged during use.
class TSNLNodeProxy final: public TNodeProxy {
  public:
    explicit TSNLNodeProxy(const naja::NL::SNLOccurrence& occurrence): occurrence_(occurrence) {}
    const naja::NL::SNLOccurrence& getOccurrence() const { return occurrence_; }
  private:
    naja::NL::SNLOccurrence occurrence_;
};

class TSNLEdgeProxy final: public TEdgeProxy {
  public:
    enum class Kind { Connectivity, Combinational, ClockToOutput };
    TSNLEdgeProxy(Kind kind, const naja::NL::SNLOccurrence& occurrence)
      : kind_(kind), occurrence_(occurrence) {}
    Kind getKind() const { return kind_; }
    /// Net occurrence for connectivity, instance occurrence for primitive arcs.
    const naja::NL::SNLOccurrence& getOccurrence() const { return occurrence_; }
  private:
    Kind kind_;
    naja::NL::SNLOccurrence occurrence_;
};

/// Builds an occurrence-aware graph from a structural SNL design.
/// Hierarchical instance terminals and their internal port occurrences share
/// one node. Leaf nodes use instance-selected modeling roles and timing arcs.
/// Supports modeled combinational cells and single-clock flip-flops; rejects
/// latches, memories, inouts, recursive hierarchy, ambiguous drivers, and
/// missing/inconsistent leaf modeling with NLException.
/// Clock definitions are not registered by this constructor.
class TSNLGraphConstructor {
  public:
    explicit TSNLGraphConstructor(naja::NL::SNLDesign* top): top_(top) {}
    /// Replaces the previous result only on success. Rebuilding invalidates
    /// references to that previous graph. Netlist edits require rebuilding.
    void construct();
    TGraph& getGraph();
    const TGraph& getGraph() const;
    /// Lookup top ports or instance terminals in their containing path.
    /// Internal hierarchical port occurrences are accepted as aliases.
    /// The mapping describes the constructed graph; do not remove/reuse its nodes.
    TGraph::NodeId getNode(const naja::NL::SNLOccurrence& occurrence) const;

  private:
    naja::NL::SNLDesign* top_;
    std::unique_ptr<TGraph> graph_;
    std::map<naja::NL::SNLOccurrence, TGraph::NodeId> nodes_;
};

}  // namespace naja::TG
