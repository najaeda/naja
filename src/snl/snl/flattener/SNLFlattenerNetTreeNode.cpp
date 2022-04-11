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

namespace naja { namespace SNL {

SNLFlattenerNetTreeNode::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

SNLFlattenerNetTreeNode::SNLFlattenerNetTreeNode(SNLFlattenerNetTree* tree, const SNLBitNet* rootNet):
  parent_(tree),
  type_(Type::Root),
  object_(rootNet)
{}

SNLFlattenerNetTreeNode::SNLFlattenerNetTreeNode(SNLFlattenerNetTreeNode* parent, const SNLInstTerm* instTerm):
  parent_(parent),
  type_(Type::InstTerm),
  object_(instTerm)
{}

SNLFlattenerNetTreeNode::SNLFlattenerNetTreeNode(SNLFlattenerNetTreeNode* parent, const SNLBitTerm* term):
  parent_(parent),
  type_(Type::Term),
  object_(term)
{}

SNLFlattenerNetTreeNode* SNLFlattenerNetTreeNode::create(SNLFlattenerNetTree* tree, const SNLBitNet* rootNet) {
  auto node = new SNLFlattenerNetTreeNode(tree, rootNet);
  return node;
}

SNLFlattenerNetTreeNode* SNLFlattenerNetTreeNode::create(SNLFlattenerNetTreeNode* parent, const SNLInstTerm* instTerm) {
  auto node = new SNLFlattenerNetTreeNode(parent, instTerm);
  return node;
}

SNLFlattenerNetTreeNode* SNLFlattenerNetTreeNode::create(SNLFlattenerNetTreeNode* parent, const SNLBitTerm* term) {
  auto node = new SNLFlattenerNetTreeNode(parent, term);
  return node;
}

SNLFlattenerNetTree* SNLFlattenerNetTreeNode::getTree() const {
  if (isRoot()) {
    return static_cast<SNLFlattenerNetTree*>(parent_);
  }
  return getParent()->getTree();
}

SNLFlattenerNetTreeNode* SNLFlattenerNetTreeNode::getParent() const {
  if (not isRoot()) {
   return static_cast<SNLFlattenerNetTreeNode*>(parent_);
  }
  return nullptr;
}

//LCOV_EXCL_START
void SNLFlattenerNetTreeNode::print(std::ostream& stream, unsigned indent) const {
  stream << std::string(indent, ' ') << getString() << std::endl;
  indent += 2;
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLFlattenerNetTreeNode::getString() const {
  return std::string();
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja