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

#include <iostream>

#include "SNLDesign.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"

#include "SNLFlattenerInstanceTree.h"
#include "SNLFlattenerNetForest.h"
#include "SNLFlattenerNetTree.h"
#include "SNLFlattenerNetTreeNode.h"
#include "SNLFlattenerInstanceTreeNode.h"

namespace naja { namespace SNL {

SNLFlattener::~SNLFlattener() {
  delete tree_;
  delete forest_;
}

void SNLFlattener::processDesign(
  SNLFlattenerInstanceTreeNode* parent,
  const SNLDesign* design,
  const TermNodesMap& termNodesMap) {
  for (auto net: design->getBitNets()) {
    if (net->getType().isConst0()) {

    } else if (net->getType().isConst1()) {

    } else {
      for (auto component: net->getComponents()) {
        if (auto term = dynamic_cast<SNLBitTerm*>(component)) {
          std::cerr << term->getString() << std::endl;
        } else {
          auto instTerm = static_cast<SNLInstTerm*>(component);
          std::cerr << instTerm->getString() << std::endl;
        }
      }
    }
  }
  for (auto instance: design->getInstances()) {
    TermNodesMap termNodesMap;
    processInstance(parent, instance, termNodesMap);
  }
}

void SNLFlattener::processInstance(
  SNLFlattenerInstanceTreeNode* parent,
  const SNLInstance* instance,
  const TermNodesMap& termNodesMap) {
  auto node = parent->addChild(instance);
  SNLDesign* model = instance->getModel();
  processDesign(node, model, termNodesMap);
}

void SNLFlattener::processTopNets(const SNLDesign* top) {
  for (auto net: top->getBitNets()) {
    for (auto component: net->getComponents()) {
      SNLFlattenerNetTree* tree = SNLFlattenerNetTree::create(getNetForest(), net);
      SNLFlattenerNetTreeNode* root = tree->getRoot();
      if (auto instTerm = dynamic_cast<SNLInstTerm*>(component)) {
        SNLFlattenerNetTreeNode::create(root, instTerm);
      } else {
        auto term = dynamic_cast<SNLBitTerm*>(component);
        SNLFlattenerNetTreeNode::create(root, term);
      }
    }
  }
}

void SNLFlattener::processTop(const SNLDesign* top) {
  auto root = tree_->getRoot();
  for (auto instance: top->getInstances()) {
    TermNodesMap termNodesMap;
    processInstance(root, instance, termNodesMap);
  }
}

void SNLFlattener::process(const SNLDesign* top) {
  tree_ = new SNLFlattenerInstanceTree(top);
  forest_ = new SNLFlattenerNetForest();
  processTop(top);
}

}} // namespace SNL // namespace naja
