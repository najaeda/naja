// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace naja::TG {

class TNodeProxy;
class TGraph;

class TNode {
  public:
    using EdgeId = std::size_t;

    ~TNode();

    std::size_t getIndex() const { return index_; }
    bool isAlive() const { return alive_; }

    TNodeProxy* getProxy() { return proxy_.get(); }
    const TNodeProxy* getProxy() const { return proxy_.get(); }

    const std::vector<EdgeId>& getInEdges() const { return inEdges_; }
    const std::vector<EdgeId>& getOutEdges() const { return outEdges_; }
    std::size_t getInDegree() const { return inEdges_.size(); }
    std::size_t getOutDegree() const { return outEdges_.size(); }

    TNode(TNode&&);
    TNode& operator=(TNode&&);

    static TNode create(std::size_t index, TNodeProxy* proxy);

  private:
    friend class TGraph;

    TNode(std::size_t index, TNodeProxy* proxy);
    TNode(const TNode&) = delete;
    TNode& operator=(const TNode&) = delete;

    void reset(std::size_t index, TNodeProxy* proxy);
    void deactivate();
    void addInEdge(EdgeId edge);
    void addOutEdge(EdgeId edge);
    void removeInEdge(EdgeId edge);
    void removeOutEdge(EdgeId edge);

    std::size_t index_ {0};
    bool alive_ {false};
    std::unique_ptr<TNodeProxy> proxy_;
    std::vector<EdgeId> inEdges_;
    std::vector<EdgeId> outEdges_;
};

}  // namespace naja::TG
