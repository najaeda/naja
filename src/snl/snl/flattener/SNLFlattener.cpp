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
#include <list>

#include "SNLDesign.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"

#include "SNLFlattenerInstanceTree.h"
#include "SNLFlattenerNetForest.h"
#include "SNLFlattenerNetTree.h"
#include "SNLFlattenerNetTreeNode.h"
#include "SNLFlattenerInstanceTreeNode.h"

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

SNLFlattener::~SNLFlattener() {
  delete tree_;
  delete forest_;
}


void SNLFlattener::processTopNets(SNLFlattenerInstanceTreeNode* instanceTreeRoot, const SNLDesign* top) {
  for (auto net: top->getBitNets()) {
    SNLFlattenerNetTree* netTree = new SNLFlattenerNetTree(getNetForest(), instanceTreeRoot, net);
    SNLFlattenerNetTreeNode* netTreeRoot = netTree->getRoot();
    setTreeType(netTree, net);
    for (auto component: net->getComponents()) {
      if (auto instTerm = dynamic_cast<SNLInstTerm*>(component)) {
        auto instance = instTerm->getInstance();
        auto instanceTreeNode = instanceTreeRoot->getChildNode(instance);
        assert(instanceTreeNode);
        new SNLFlattenerNetTreeNode(netTreeRoot, instanceTreeNode, instTerm);
      } else {
        auto term = static_cast<SNLBitTerm*>(component);
        new SNLFlattenerNetTreeNode(netTreeRoot, term);
      }
    }
  }
}

void SNLFlattener::createNetForest(SNLFlattenerInstanceTreeNode* instanceNode) {
  auto instance = instanceNode->getInstance();
  auto model = instance->getModel();
  for (auto bitNet: model->getBitNets()) {
    using InstTerms = std::list<SNLInstTerm*>;
    InstTerms instTerms;
    using TermNodes = std::list<SNLFlattenerNetTreeNode*>;
    TermNodes termNodes;
    for (auto component: bitNet->getComponents()) {
      if (auto instTerm = dynamic_cast<SNLInstTerm*>(component)) {
        instTerms.push_back(instTerm);
      } else {
        auto bitTerm = static_cast<SNLBitTerm*>(component);
        auto termNode = instanceNode->getTermNode(bitTerm);
        assert(termNode);
        termNodes.push_back(termNode);
      }
      SNLFlattenerNetTreeNode* parentNode = nullptr;
      if (termNodes.empty()) {
        //new net tree
        auto netTree = new SNLFlattenerNetTree(getNetForest(), instanceNode, bitNet);
        parentNode = netTree->getRoot();
      } else {
        //manage feedthrus....
        assert(termNodes.size() == 1);
        parentNode = termNodes.front();
      }
      for (auto instTerm: instTerms) {
        auto instance = instTerm->getInstance();
        auto instanceChildNode = instanceNode->getChildNode(instance);
        assert(instanceChildNode);
        new SNLFlattenerNetTreeNode(parentNode, instanceChildNode, instTerm);
      }
    }
  }
}

void SNLFlattener::createNetForest(const SNLDesign* top) {
  forest_ = new SNLFlattenerNetForest;
  auto tree = getInstanceTree();
  auto root = tree->getRoot();
  processTopNets(root, top);
  for (auto instance: top->getInstances()) {
    auto instanceNode = root->getChildNode(instance);
    assert(instanceNode);
    createNetForest(instanceNode);
  }
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
