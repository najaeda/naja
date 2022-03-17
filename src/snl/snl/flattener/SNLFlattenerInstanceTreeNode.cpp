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

void SNLFlattenerInstanceTreeNode::print(std::ostream& stream, unsigned indent) const {
  stream << std::string(indent, ' ') << getString() << std::endl;
  indent += 2;
  for (const auto& [instance, child]: children_) {
    child->print(stream, indent);
  }
}

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

}} // namespace SNL // namespace naja
