#include "SNLFlattenerInstanceTreeNode.h"

namespace SNL {

SNLFlattenerInstanceTreeNode::SNLFlattenerInstanceTreeNode(
  SNLFlattenerInstanceTreeNode* parent,
  const SNLInstance* instance):
  isRoot_(false),
  instance_(instance)
{}

void SNLFlattenerInstanceTreeNode::addChild(const SNLInstance* instance) {
  SNLFlattenerInstanceTreeNode* child = new SNLFlattenerInstanceTreeNode(this, instance);
  children_[instance] = child;
}

SNLFlattenerInstanceTreeNode::~SNLFlattenerInstanceTreeNode() {
  std::for_each(children_.begin(), children_.end(), [](const auto& pair){ delete pair.second; });
}

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