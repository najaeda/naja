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

#include "SNLFlattener.h"

#include "SNLDesign.h"

#include "SNLFlattenerInstanceTree.h"
#include "SNLFlattenerInstanceTreeNode.h"


namespace naja { namespace SNL {

SNLFlattener::~SNLFlattener() {
  delete tree_;
}

void SNLFlattener::processDesign(SNLFlattenerInstanceTreeNode* parent, const SNLDesign* design) {
  for (auto instance: design->getInstances()) {
    processInstance(parent, instance);
  }
}

void SNLFlattener::processInstance(SNLFlattenerInstanceTreeNode* parent, const SNLInstance* instance) {
  auto node = parent->addChild(instance);
  SNLDesign* model = instance->getModel();
  processDesign(node, model);
}

void SNLFlattener::processTop(const SNLDesign* top) {
  auto root = tree_->getRoot();
  for (auto instance: top->getInstances()) {
    processInstance(root, instance);
  }
}

void SNLFlattener::process(const SNLDesign* top) {
  tree_ = SNLFlattenerInstanceTree::create(top);
  processTop(top);
}

}} // namespace SNL // namespace naja
