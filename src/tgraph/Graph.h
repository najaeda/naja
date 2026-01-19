#pragma once

#include <cstddef>
#include <vector>

#include "Edge.h"
#include "Node.h"

namespace naja::TG {

class EdgeProxy;
class NodeProxy;

class Graph {
  public:
    using NodeId = std::size_t;
    using EdgeId = std::size_t;

    static constexpr NodeId kInvalidNode = static_cast<NodeId>(-1);
    static constexpr EdgeId kInvalidEdge = static_cast<EdgeId>(-1);

    Graph() = default;
    ~Graph() = default;

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    Graph(Graph&&) = delete;
    Graph& operator=(Graph&&) = delete;

    NodeId createNode(NodeProxy* proxy);
    EdgeId createEdge(NodeId source, NodeId target, EdgeProxy* proxy);

    bool removeNode(NodeId node);
    bool removeEdge(EdgeId edge);

    Node* getNode(NodeId node);
    const Node* getNode(NodeId node) const;

    Edge* getEdge(EdgeId edge);
    const Edge* getEdge(EdgeId edge) const;

    std::size_t getNodesCount() const { return liveNodes_; }
    std::size_t getEdgesCount() const { return liveEdges_; }

    const std::vector<Node>& getNodesStorage() const { return nodes_; }
    const std::vector<Edge>& getEdgesStorage() const { return edges_; }

    bool isNodeValid(NodeId node) const;
    bool isEdgeValid(EdgeId edge) const;

  private:
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::vector<NodeId> freeNodes_;
    std::vector<EdgeId> freeEdges_;
    std::size_t liveNodes_ {0};
    std::size_t liveEdges_ {0};
};

}  // namespace naja::TG
