#include "Node.h"

#include "NodeProxy.h"

namespace naja::TG {

Node::~Node() {
  delete proxy_;
}

}  // namespace naja::TG