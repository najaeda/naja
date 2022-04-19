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

namespace naja { namespace SNL {

SNLFlattenerNetTree::SNLFlattenerNetTree(SNLFlattenerNetForest* forest, const SNLBitNet* net):
  forest_(forest) {
  root_ = SNLFlattenerNetTreeNode::create(this, net);
  forest_->addTree(this);
}

void SNLFlattenerNetTree::destroyFromForest() {
  delete this;
}

void SNLFlattenerNetTree::destroy() {
  forest_->removeTree(this);
  delete this;  
}

SNLFlattenerNetTree* SNLFlattenerNetTree::create(SNLFlattenerNetForest* forest, const SNLBitNet* net) {
  SNLFlattenerNetTree* tree = new SNLFlattenerNetTree(forest, net);
  return tree;
}

void SNLFlattenerNetTree::print(std::ostream& stream) const {
  getRoot()->print(stream);
}

}} // namespace SNL // namespace naja