#include "SNLFlattenerInstanceTreeNode.h"

namespace SNL {

SNLFlattenerInstanceTreeNode* SNLFlattenerInstanceTreeNode::getParent() const {
  if (isRoot()) {
    return nullptr;
  }
  return static_cast<SNLFlattenerInstanceTreeNode*>(parent_);
}

SNLFlattenerInstanceTree* SNLFlattenerInstanceTreeNode::getTree() const {
  if (isRoot()) {
    return static_cast<SNLFlattenerInstanceTree*>(parent_);
  }
  return getParent()->getTree();
}

}