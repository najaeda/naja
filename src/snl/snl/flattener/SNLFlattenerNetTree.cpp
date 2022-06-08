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

#include "SNLFlattenerNetTree.h"

#include "SNLFlattenerNetForest.h"
#include "SNLFlattenerNetTreeNode.h"

namespace {

void setTreeType(naja::SNL::SNLFlattenerNetTree* tree, const naja::SNL::SNLBitNet* net) {
  if (net->isConstant0()) {
    tree->setType(naja::SNL::SNLFlattenerNetTree::Type::Constant0);
  } else if (net->isConstant1()) {
    tree->setType(naja::SNL::SNLFlattenerNetTree::Type::Constant1);
  }
}

}

namespace naja { namespace SNL {

SNLFlattenerNetTree::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string SNLFlattenerNetTree::Type::getString() const {
  switch (typeEnum_) {
    case Type::Standard:  return "Standard";
    case Type::Constant0: return "Constant0";
    case Type::Constant1: return "Constant1";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

SNLFlattenerNetTree::SNLFlattenerNetTree(
  SNLFlattenerNetForest* forest,
  SNLFlattenerInstanceTreeNode* instanceTreeNode,
  const SNLBitNet* net):
  forest_(forest) {
  root_ = new SNLFlattenerNetTreeNode(this, instanceTreeNode, net);
  setTreeType(this, net);
  forest_->addTree(this);
}

SNLFlattenerNetTree::~SNLFlattenerNetTree() {
  if (forest_) {
    forest_->removeTree(this);
  }
  if (root_) {
    root_->parent_ = nullptr;
    delete root_;
  }
}

//LCOV_EXCL_START
void SNLFlattenerNetTree::print(std::ostream& stream) const {
  getRoot()->print(stream);
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja