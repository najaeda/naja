#include "Graph.h"

#include <vector>

#include "Edge.h"
#include "Node.h"

namespace naja::TG {

Graph::NodeId Graph::createNode(NodeProxy* proxy) {
  NodeId id = kInvalidNode;
  if (!freeNodes_.empty()) {
    id = freeNodes_.back();
    freeNodes_.pop_back();
    nodes_[id].reset(id, proxy);
  } else {
    id = nodes_.size();
    nodes_.emplace_back(Node::create(id, proxy));
  }
  ++liveNodes_;
  return id;
}

Graph::EdgeId Graph::createEdge(NodeId source, NodeId target, EdgeProxy* proxy) {
  if (!isNodeValid(source) || !isNodeValid(target)) {
    return kInvalidEdge;
  }

  EdgeId id = kInvalidEdge;
  if (!freeEdges_.empty()) {
    id = freeEdges_.back();
    freeEdges_.pop_back();
    edges_[id].reset(id, source, target, proxy);
  } else {
    id = edges_.size();
    edges_.emplace_back(Edge::create(id, source, target, proxy));
  }

  nodes_[source].addOutEdge(id);
  nodes_[target].addInEdge(id);
  ++liveEdges_;
  return id;
}

bool Graph::removeEdge(EdgeId edge) {
  if (!isEdgeValid(edge)) {
    return false;
  }

  Edge& edgeRef = edges_[edge];
  const NodeId source = edgeRef.getSource();
  const NodeId target = edgeRef.getTarget();
  if (isNodeValid(source)) {
    nodes_[source].removeOutEdge(edge);
  }
  if (isNodeValid(target)) {
    nodes_[target].removeInEdge(edge);
  }

  edgeRef.deactivate();
  freeEdges_.push_back(edge);
  --liveEdges_;
  return true;
}

bool Graph::removeNode(NodeId node) {
  if (!isNodeValid(node)) {
    return false;
  }

  Node& nodeRef = nodes_[node];
  const std::vector<EdgeId> outEdges = nodeRef.getOutEdges();
  const std::vector<EdgeId> inEdges = nodeRef.getInEdges();
  for (EdgeId edge : outEdges) {
    removeEdge(edge);
  }
  for (EdgeId edge : inEdges) {
    removeEdge(edge);
  }

  nodeRef.deactivate();
  freeNodes_.push_back(node);
  --liveNodes_;
  return true;
}

Node* Graph::getNode(NodeId node) {
  if (!isNodeValid(node)) {
    return nullptr;
  }
  return &nodes_[node];
}

const Node* Graph::getNode(NodeId node) const {
  if (!isNodeValid(node)) {
    return nullptr;
  }
  return &nodes_[node];
}

Edge* Graph::getEdge(EdgeId edge) {
  if (!isEdgeValid(edge)) {
    return nullptr;
  }
  return &edges_[edge];
}

const Edge* Graph::getEdge(EdgeId edge) const {
  if (!isEdgeValid(edge)) {
    return nullptr;
  }
  return &edges_[edge];
}

bool Graph::isNodeValid(NodeId node) const {
  return node < nodes_.size() && nodes_[node].isAlive();
}

bool Graph::isEdgeValid(EdgeId edge) const {
  return edge < edges_.size() && edges_[edge].isAlive();
}

}  // namespace naja::TG
