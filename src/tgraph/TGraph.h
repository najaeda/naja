// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "TClock.h"
#include "TEdge.h"
#include "TNode.h"

namespace naja::NL {

class SNLNetComponent;

}  // namespace naja::NL

namespace naja::TG {

class TEdgeProxy;
class TNodeProxy;

class TGraph {
  public:
    using NodeId = std::size_t;
    using EdgeId = std::size_t;
    using ClockId = TClock::ClockId;

    static constexpr NodeId kInvalidNode = static_cast<NodeId>(-1);
    static constexpr EdgeId kInvalidEdge = static_cast<EdgeId>(-1);

    TGraph() = default;
    ~TGraph() = default;

    TGraph(const TGraph&) = delete;
    TGraph& operator=(const TGraph&) = delete;
    TGraph(TGraph&&) = delete;
    TGraph& operator=(TGraph&&) = delete;

    NodeId createNode(TNodeProxy* proxy);
    EdgeId createEdge(NodeId source, NodeId target, TEdgeProxy* proxy);

    bool removeNode(NodeId node);
    bool removeEdge(EdgeId edge);

    /// \warning the returned pointer is a transient view into vector-backed
    /// storage: it is invalidated by any later createNode()/createEdge()
    /// call that reallocates that storage. NodeId is the stable handle;
    /// re-fetch getNode() after any graph mutation rather than caching it.
    TNode* getNode(NodeId node);
    const TNode* getNode(NodeId node) const;

    /// \warning see getNode()'s pointer-invalidation warning; EdgeId is the
    /// stable handle, not the returned pointer.
    TEdge* getEdge(EdgeId edge);
    const TEdge* getEdge(EdgeId edge) const;

    std::size_t getNodesCount() const { return liveNodes_; }
    std::size_t getEdgesCount() const { return liveEdges_; }

    const std::vector<TNode>& getNodesStorage() const { return nodes_; }
    const std::vector<TEdge>& getEdgesStorage() const { return edges_; }

    bool isNodeValid(NodeId node) const;
    bool isEdgeValid(EdgeId edge) const;

    /// \brief register a clock reaching `target` (a top terminal or an
    /// instance terminal), seeded at `node` - the graph node TClockPropagator
    /// starts its forward pass from for this clock. The number of clocks is
    /// unbounded. Grows every live edge's FromClocks/ToClocks masks to
    /// match, so all edges always carry same-size masks, sized to
    /// getClocksCount().
    /// \throws std::invalid_argument if `node` is not a valid node.
    ClockId createClock(const std::string& name, naja::NL::SNLNetComponent* target, NodeId node);

    const TClock* getClock(ClockId clock) const;
    std::size_t getClocksCount() const { return clocks_.size(); }
    const std::vector<TClock>& getClocksStorage() const { return clocks_; }
    bool isClockValid(ClockId clock) const { return clock < clocks_.size(); }

  private:
    std::vector<TNode> nodes_;
    std::vector<TEdge> edges_;
    std::vector<NodeId> freeNodes_;
    std::vector<EdgeId> freeEdges_;
    std::vector<TClock> clocks_;
    std::size_t liveNodes_ {0};
    std::size_t liveEdges_ {0};
};

}  // namespace naja::TG
