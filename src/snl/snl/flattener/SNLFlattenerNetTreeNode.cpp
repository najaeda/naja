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

#include "SNLFlattenerNetTreeNode.h"

#include <sstream>

#include "SNLFlattenerNetTree.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"

namespace naja { namespace SNL {

SNLFlattenerNetTreeNode::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string SNLFlattenerNetTreeNode::Type::getString() const {
  switch (typeEnum_) {
    case Type::Root:      return "Root";
    case Type::InstTerm:  return "InstTerm";
    case Type::Term:      return "Term";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

SNLFlattenerNetTreeNode::SNLFlattenerNetTreeNode(
  SNLFlattenerNetTree* tree,
  SNLFlattenerInstanceTreeNode* instanceTreeNode,
  const SNLBitNet* rootNet):
  parent_(tree),
  instanceTreeNode_(instanceTreeNode),
  type_(Type::Root),
  object_(rootNet)
{
  instanceTreeNode_->addNetNode(this, rootNet); 
}

SNLFlattenerNetTreeNode::SNLFlattenerNetTreeNode(
  SNLFlattenerNetTreeNode* parent,
  SNLFlattenerInstanceTreeNode* instanceTreeNode,
  const SNLInstTerm* instTerm):
  parent_(parent),
  instanceTreeNode_(instanceTreeNode),
  type_(Type::InstTerm),
  object_(instTerm)
{
  parent->addChild(this, object_); 
  instanceTreeNode_->addTermNode(this, instTerm->getTerm());
}

SNLFlattenerNetTreeNode::SNLFlattenerNetTreeNode(SNLFlattenerNetTreeNode* parent, const SNLBitTerm* term):
  parent_(parent),
  type_(Type::Term),
  object_(term)
{
  parent->addChild(this, object_);
}

SNLFlattenerNetTreeNode::~SNLFlattenerNetTreeNode() {
  if (parent_) {
    if (isRoot()) {
      //this tree needs to be deleted
      auto tree = getTree();
      tree->root_ = nullptr;
      delete tree;
    } else {
      auto parent = getParent();
      parent->removeChild(this);
    }
  }
  for (const auto& [object, child]: children_) {
    child->parent_ = nullptr;
    delete child;
  }
}

void SNLFlattenerNetTreeNode::addChild(
  SNLFlattenerNetTreeNode* child,
  const SNLDesignObject* object) {
  children_[object] = child;
}

void SNLFlattenerNetTreeNode::removeChild(SNLFlattenerNetTreeNode* child) {
  auto it = children_.find(child->object_);
  assert(it != children_.end());
  children_.erase(it);
}

SNLFlattenerNetTree* SNLFlattenerNetTreeNode::getTree() const {
  if (isRoot()) {
    return static_cast<SNLFlattenerNetTree*>(parent_);
  }
  return getParent()->getTree();
}

SNLFlattenerNetForest* SNLFlattenerNetTreeNode::getForest() const {
  return getTree()->getForest();
}

SNLFlattenerNetTreeNode* SNLFlattenerNetTreeNode::getParent() const {
  if (not isRoot()) {
    return static_cast<SNLFlattenerNetTreeNode*>(parent_);
  }
  return nullptr;
}

const SNLBitNet* SNLFlattenerNetTreeNode::getNet() const {
  if (isRoot()) {
    return static_cast<const SNLBitNet*>(object_);
  }
  return nullptr;
}

const SNLBitTerm* SNLFlattenerNetTreeNode::getTerm() const {
  if (isTerm()) {
    return static_cast<const SNLBitTerm*>(object_);
  }
  return nullptr;
}

const SNLInstTerm* SNLFlattenerNetTreeNode::getInstTerm() const {
  if (isInstTerm()) {
    return static_cast<const SNLInstTerm*>(object_);
  }
  return nullptr;
}

//LCOV_EXCL_START
std::string SNLFlattenerNetTreeNode::getObjectString() const {
  switch (getType()) {
    case Type::Root: {
      auto net = getNet();
      return net->getString();
    }
    case Type::Term: {
      auto term = getTerm();
      return term->getString();
    }
    case Type::InstTerm: {
      auto instTerm = getInstTerm();
      return instTerm->getString();
    }
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLFlattenerNetTreeNode::print(std::ostream& stream, unsigned indent) const {
  stream << std::string(indent, ' ') << getString() << std::endl;
  indent += 2;
  for (const auto& [object, child]: children_) {
    child->print(stream, indent);
  }
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLFlattenerNetTreeNode::getString() const {
  std::ostringstream stream;
  stream << getType().getString();
  stream << ":" << getObjectString();
  return stream.str();
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja