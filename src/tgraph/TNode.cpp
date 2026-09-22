// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TNode.h"

#include "TNodeProxy.h"

namespace naja::TG {

namespace {

void eraseEdgeId(std::vector<TNode::EdgeId>& edges, TNode::EdgeId edge) {
  for (std::size_t i = 0; i < edges.size(); ++i) {
    if (edges[i] == edge) {
      edges[i] = edges.back();
      edges.pop_back();
      return;
    }
  }
}

}  // namespace

TNode::TNode(std::size_t index, TNodeProxy* proxy)
  : index_(index),
    alive_(true),
    proxy_(proxy) {
}

TNode::TNode(TNode&&) = default;
TNode& TNode::operator=(TNode&&) = default;

TNode::~TNode() = default;

TNode TNode::create(std::size_t index, TNodeProxy* proxy) {
  return TNode(index, proxy);
}

void TNode::reset(std::size_t index, TNodeProxy* proxy) {
  index_ = index;
  alive_ = true;
  proxy_.reset(proxy);
  inEdges_.clear();
  outEdges_.clear();
}

void TNode::deactivate() {
  alive_ = false;
  proxy_.reset();
  inEdges_.clear();
  outEdges_.clear();
}

void TNode::addInEdge(EdgeId edge) {
  inEdges_.push_back(edge);
}

void TNode::addOutEdge(EdgeId edge) {
  outEdges_.push_back(edge);
}

void TNode::removeInEdge(EdgeId edge) {
  eraseEdgeId(inEdges_, edge);
}

void TNode::removeOutEdge(EdgeId edge) {
  eraseEdgeId(outEdges_, edge);
}

}  // namespace naja::TG
