#include "Node.h"

#include "NodeProxy.h"

namespace naja::TG {

namespace {

void eraseEdgeId(std::vector<Node::EdgeId>& edges, Node::EdgeId edge) {
  for (std::size_t i = 0; i < edges.size(); ++i) {
    if (edges[i] == edge) {
      edges[i] = edges.back();
      edges.pop_back();
      return;
    }
  }
}

}  // namespace

Node::Node(std::size_t index, NodeProxy* proxy)
  : index_(index),
    alive_(true),
    proxy_(proxy) {
}

Node::Node(Node&&) = default;
Node& Node::operator=(Node&&) = default;

Node::~Node() = default;

Node Node::create(std::size_t index, NodeProxy* proxy) {
  return Node(index, proxy);
}

void Node::reset(std::size_t index, NodeProxy* proxy) {
  index_ = index;
  alive_ = true;
  proxy_.reset(proxy);
  inEdges_.clear();
  outEdges_.clear();
}

void Node::deactivate() {
  alive_ = false;
  proxy_.reset();
  inEdges_.clear();
  outEdges_.clear();
}

void Node::addInEdge(EdgeId edge) {
  inEdges_.push_back(edge);
}

void Node::addOutEdge(EdgeId edge) {
  outEdges_.push_back(edge);
}

void Node::removeInEdge(EdgeId edge) {
  eraseEdgeId(inEdges_, edge);
}

void Node::removeOutEdge(EdgeId edge) {
  eraseEdgeId(outEdges_, edge);
}

}  // namespace naja::TG
