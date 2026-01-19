#include "Edge.h"

#include "EdgeProxy.h"

namespace naja::TG {

Edge::Edge(EdgeId index, NodeId source, NodeId target, EdgeProxy* proxy)
  : index_(index),
    source_(source),
    target_(target),
    alive_(true),
    proxy_(proxy) {
}

Edge::Edge(Edge&&) = default;
Edge& Edge::operator=(Edge&&) = default;

Edge::~Edge() = default;

Edge Edge::create(EdgeId index, NodeId source, NodeId target, EdgeProxy* proxy) {
  return Edge(index, source, target, proxy);
}

void Edge::reset(EdgeId index, NodeId source, NodeId target, EdgeProxy* proxy) {
  index_ = index;
  source_ = source;
  target_ = target;
  alive_ = true;
  proxy_.reset(proxy);
}

void Edge::deactivate() {
  alive_ = false;
  proxy_.reset();
}

}  // namespace naja::TG
