#pragma once

#include <cstddef>
#include <memory>

namespace naja::TG {

class EdgeProxy;
class Graph;

class Edge {
  public:
    using NodeId = std::size_t;
    using EdgeId = std::size_t;

    ~Edge();

    EdgeId getIndex() const { return index_; }
    NodeId getSource() const { return source_; }
    NodeId getTarget() const { return target_; }
    bool isAlive() const { return alive_; }

    EdgeProxy* getProxy() { return proxy_.get(); }
    const EdgeProxy* getProxy() const { return proxy_.get(); }

    Edge(Edge&&);
    Edge& operator=(Edge&&);

    static Edge create(EdgeId index, NodeId source, NodeId target, EdgeProxy* proxy);

  private:
    friend class Graph;

    Edge(EdgeId index, NodeId source, NodeId target, EdgeProxy* proxy);
    Edge(const Edge&) = delete;
    Edge& operator=(const Edge&) = delete;

    void reset(EdgeId index, NodeId source, NodeId target, EdgeProxy* proxy);
    void deactivate();

    EdgeId index_ {0};
    NodeId source_ {0};
    NodeId target_ {0};
    bool alive_ {false};
    std::unique_ptr<EdgeProxy> proxy_;
};

}  // namespace naja::TG
