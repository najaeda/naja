/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SNLFlattenerInstanceTreeNode.h"

#include <iostream>
#include <algorithm>

#include "SNLDesign.h"

#include "SNLFlattenerInstanceTree.h"
#include "SNLFlattenerNetTreeNode.h"

namespace naja { namespace SNL {

SNLFlattenerInstanceTreeNode::SNLFlattenerInstanceTreeNode(
  SNLFlattenerInstanceTree* tree,
  const SNLDesign* top):
  isRoot_(true),
  parent_(tree),
  object_(top)
{}

SNLFlattenerInstanceTreeNode::SNLFlattenerInstanceTreeNode(
  SNLFlattenerInstanceTreeNode* parent,
  const SNLInstance* instance):
  isRoot_(false),
  parent_(parent),
  object_(instance)
{}

SNLFlattenerInstanceTreeNode* SNLFlattenerInstanceTreeNode::addChild(const SNLInstance* instance) {
  SNLFlattenerInstanceTreeNode* child = new SNLFlattenerInstanceTreeNode(this, instance);
  children_[instance] = child;
  return child;
}

void SNLFlattenerInstanceTreeNode::removeChild(const SNLInstance* instance) {
  auto it = children_.find(instance);
  if (it!=children_.end()) {
    children_.erase(it);
  }
}

void SNLFlattenerInstanceTreeNode::addNetNode(SNLFlattenerNetTreeNode* node, const SNLBitNet* net) {
  netNodes_[net] = node;
}

void SNLFlattenerInstanceTreeNode::addInstTermNode(SNLFlattenerNetTreeNode* node, const SNLBitTerm* term) {
  instTermNodes_[term] = node;
}

SNLFlattenerInstanceTreeNode::~SNLFlattenerInstanceTreeNode() {
  std::for_each(children_.begin(), children_.end(), [](const auto& pair){
    auto child = pair.second;
    child->parent_ = nullptr;
    delete child;
  });
  children_.clear();
  std::for_each(netNodes_.begin(), netNodes_.end(), [](const auto& pair){
    auto node = pair.second;
    node->instanceTreeNode_ = nullptr;
    delete node;
  });
  netNodes_.clear();
  std::for_each(instTermNodes_.begin(), instTermNodes_.end(), [](const auto& pair){
    auto node = pair.second;
    node->instanceTreeNode_ = nullptr;
    delete node;
  });
  instTermNodes_.clear();
  
  if (parent_) {
    if (isRoot()) {
      getTree()->root_ = nullptr;
    } else {
      getParent()->removeChild(getInstance());
    }
  }
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

SNLFlattenerInstanceTreeNode*
SNLFlattenerInstanceTreeNode::getChildNode(const SNLInstance* instance) const {
  if (not instance) {
    return nullptr;
  }
  auto it = children_.find(instance);
  if (it != children_.end()) {
    return it->second;
  }
  return nullptr;
}

SNLFlattenerNetTreeNode*
SNLFlattenerInstanceTreeNode::getNetNode(const SNLBitNet* net) const {
  if (not net) {
    return nullptr;
  }
  auto it = netNodes_.find(net);
  if (it != netNodes_.end()) {
    return it->second;
  }
  return nullptr;
}

SNLFlattenerNetTreeNode*
SNLFlattenerInstanceTreeNode::getTermNode(const SNLBitTerm* term) const {
  if (not term) {
    return nullptr;
  }
  auto it = instTermNodes_.find(term);
  if (it != instTermNodes_.end()) {
    return it->second;
  }
  return nullptr;
}

const SNLDesign* SNLFlattenerInstanceTreeNode::getDesign() const {
  if (isRoot()) {
    return static_cast<const SNLDesign*>(object_);
  }
  return nullptr;
}

const SNLInstance* SNLFlattenerInstanceTreeNode::getInstance() const {
  if (not isRoot()) {
    return static_cast<const SNLInstance*>(object_);
  }
  return nullptr;
}

SNLCollection<SNLFlattenerInstanceTreeNode*> SNLFlattenerInstanceTreeNode::getChildren() const {
  return SNLCollection(new SNLSTLMapCollection(&children_));
}

SNLCollection<SNLFlattenerInstanceTreeNode*> SNLFlattenerInstanceTreeNode::getLeaves() const {
  auto childrenGetter = [](SNLFlattenerInstanceTreeNode* n) { return n->getChildren(); };
  auto leafCritetion = [](SNLFlattenerInstanceTreeNode* n) { return n->isLeaf(); };
  return SNLCollection(
      new SNLTreeLeavesCollection(
        const_cast<SNLFlattenerInstanceTreeNode*>(this),
        childrenGetter,
        leafCritetion));
}

//LCOV_EXCL_START
void SNLFlattenerInstanceTreeNode::print(std::ostream& stream, unsigned indent) const {
  stream << std::string(indent, ' ') << getString() << std::endl;
  indent += 2;
  for (const auto& [instance, child]: children_) {
    child->print(stream, indent);
  }
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLFlattenerInstanceTreeNode::getString() const {
  std::string str;
  if (isRoot()) {
    auto design = getDesign();
    str = std::to_string(design->getID());
    if (not design->isAnonymous()) {
      str += ":" + design->getName().getString();
    }
  } else {
    auto instance = getInstance();
    str = std::to_string(instance->getID());
    if (not instance->isAnonymous()) {
      str += ":" + instance->getName().getString();
    }
  }
  return str;
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
