#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace naja::TG {

class NodeProxy;
class Graph;

class Node {
  public:
    using EdgeId = std::size_t;

    ~Node();

    std::size_t getIndex() const { return index_; }
    bool isAlive() const { return alive_; }

    NodeProxy* getProxy() { return proxy_.get(); }
    const NodeProxy* getProxy() const { return proxy_.get(); }

    const std::vector<EdgeId>& getInEdges() const { return inEdges_; }
    const std::vector<EdgeId>& getOutEdges() const { return outEdges_; }
    std::size_t getInDegree() const { return inEdges_.size(); }
    std::size_t getOutDegree() const { return outEdges_.size(); }

    Node(Node&&);
    Node& operator=(Node&&);

    static Node create(std::size_t index, NodeProxy* proxy);

  private:
    friend class Graph;

    Node(std::size_t index, NodeProxy* proxy);
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    void reset(std::size_t index, NodeProxy* proxy);
    void deactivate();
    void addInEdge(EdgeId edge);
    void addOutEdge(EdgeId edge);
    void removeInEdge(EdgeId edge);
    void removeOutEdge(EdgeId edge);

    std::size_t index_ {0};
    bool alive_ {false};
    std::unique_ptr<NodeProxy> proxy_;
    std::vector<EdgeId> inEdges_;
    std::vector<EdgeId> outEdges_;
};

}  // namespace naja::TG
