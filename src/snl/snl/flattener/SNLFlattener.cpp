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

#if 0
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
#endif

void SNLFlattener::processTopNets(SNLFlattenerInstanceTreeNode* instanceTreeRoot, const SNLDesign* top) {
  for (auto net: top->getBitNets()) {
    SNLFlattenerNetTree* netTree = SNLFlattenerNetTree::create(getNetForest(), instanceTreeRoot, net);
    SNLFlattenerNetTreeNode* netTreeRoot = netTree->getRoot();
    for (auto component: net->getComponents()) {
      if (auto instTerm = dynamic_cast<SNLInstTerm*>(component)) {
        auto instance = instTerm->getInstance();
        auto instanceTreeNode = instanceTreeRoot->getChildNode(instance);
        assert(instanceTreeNode);
        SNLFlattenerNetTreeNode::create(netTreeRoot, instanceTreeNode, instTerm);
      } else {
        auto term = static_cast<SNLBitTerm*>(component);
        SNLFlattenerNetTreeNode::create(netTreeRoot, term);
      }
    }
  }
}

void SNLFlattener::createNetForest(const SNLDesign* top) {
  forest_ = new SNLFlattenerNetForest;
  auto tree = getInstanceTree();
  auto root = tree->getRoot();
  processTopNets(root, top);
}

void SNLFlattener::createInstanceTree(SNLFlattenerInstanceTreeNode* parent, const SNLDesign* design) {
  for (auto instance: design->getInstances()) {
    auto node = parent->addChild(instance);
    createInstanceTree(node, instance->getModel());
  }
}

void SNLFlattener::process(const SNLDesign* top) {
  tree_ = new SNLFlattenerInstanceTree(top);
  createInstanceTree(tree_->getRoot(), top);
  createNetForest(top);
}

}} // namespace SNL // namespace naja
