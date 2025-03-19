// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "ActionTree.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <stack>
#include <vector>

#include "Utils.h"
#include "NLUniverse.h"
#include "NLID.h"

#include "SNLDesign.h"
#include "SNLDesignObject.h"
#include "SNLInstance.h"
#include "SNLUniquifier.h"

using namespace naja::NL;
using namespace naja::BNE;

ActionTree::ActionTree(bool blockNormalization, bool keepOrder)
    : blockNormalization_(blockNormalization), keepOrder_(keepOrder) {
  if (not NLUniverse::get()->getTopDesign()) {
    throw NLException("cannot create ActionTree with null top design");
  }
  nodes_.push_back({0, NLUniverse::get()->getTopDesign()->getNLID(),
                    std::pair<size_t, size_t>((size_t)-1, 0), 0, this});
}

ActionTreeNode* ActionTree::getNodeForContext(
    const std::vector<NLID::DesignObjectID>& context) {
  size_t id = nodes_[0].getID();
  for (auto& instance : context) {
    ActionTreeNode* child = nodes_[id].getChild(instance);
    if (child == nullptr) {
      return nullptr;
    }
    id = child->getID();
  }
  return &nodes_[id];
}

ActionTreeNode& ActionTree::addHierChild(
    const std::vector<NLID::DesignObjectID>& context) {
  size_t id = nodes_[0].getID();
  SNLDesign* model = NLUniverse::get()->getTopDesign();
  std::vector<NLID::DesignObjectID> partialContextCheck;
  for (auto& instance : context) {
    partialContextCheck.push_back(instance);
    model = model->getInstance(instance)->getModel();
    ActionTreeNode* child = nodes_[id].getChild(instance);
    if (child == nullptr) {
      nodes_.push_back({instance, model->getNLID(),
                        std::pair<size_t, size_t>(nodes_[id].getID(), instance),
                        nodes_.size(), this});
      nodes_[id].addChild(nodes_.size() - 1);
      assert(nodes_[id].getChild(instance) != nullptr);
      assert(getNodeForContext(partialContextCheck) != nullptr);
      child = nodes_[id].getChild(instance);
      assert(child != nullptr);
    }
    id = child->getID();
  }
  assert(getNodeForContext(context) != nullptr);
  assert(getNodeForContext(context) == &nodes_[id]);
  return nodes_[id];
}

void ActionTree::addAction(Action* action) {
  actions_.push_back(action);
  std::vector<ActionID> actions;
  actions.push_back({ActionType::REDUCTION, actions_.size() - 1});
  if (action->getContext().size() > 0) {
    addHierChild(action->getContext());
  }
  // Add action to node
  getNodeForContext(action->getContext())->addActions(actions);
}

void ActionTree::normalize() {
  // Sort actions on all nodes
  if (!keepOrder_) {
    for (auto& node : nodes_) {
      node.sortActions();
    }
  }
  // Collect in dfs manner all the tree nodes into a map with key of snlid
  std::map<NLID, std::vector<ActionTreeNode*>> snlid2Node;
  std::vector<ActionTreeNode*> stack;
  stack.push_back(&nodes_[0]);
  std::vector<NLID> snlidorder;
  while (!stack.empty()) {
    ActionTreeNode* currentNode = stack.back();
    stack.pop_back();
    if (snlid2Node.find(currentNode->getNLID()) != snlid2Node.end()) {
      assert(getInstanceForPath(
                 snlid2Node[currentNode->getNLID()].back()->getContext())
                 ->getModel() ==
             (getInstanceForPath(currentNode->getContext()))->getModel());
    }
    snlid2Node[currentNode->getNLID()].push_back(currentNode);
    snlidorder.push_back(currentNode->getNLID());

    for (auto& child : currentNode->getChildren()) {
      stack.push_back(&nodes_[child]);
    }
  }
  for (auto& snlid : snlidorder) {
    auto nodes = snlid2Node[snlid];
    // If top, continue
    if (nodes[0]->getParents()[0].first == (size_t)-1) {
      continue;
    }
    // bool foundNormalization = true;
    while (/*foundNormalization && <- not true as the currrent node have no
              merge but this does not mean there is no merge for others*/
           nodes.size() > 1) {
      // foundNormalization = false;
      std::vector<ActionTreeNode*> nodesToMerge;
      auto currentNode = nodes.back();
      nodes.pop_back();
      if (!currentNode->isPartOfTree()) {
        continue;
      }
      nodesToMerge.push_back(currentNode);
      for (auto node : nodes) {
        if (node == currentNode) {
          continue;
        }
        if (!node->isPartOfTree()) {
          continue;
        }
        if (*currentNode == *node) {
          nodesToMerge.push_back(node);
          // foundNormalization = true;
        }
      }
      for (auto node : nodesToMerge) {
        if (node == currentNode) {
          continue;
        }
        nodes.erase(std::find(nodes.begin(), nodes.end(), node));
        assert(node->getParents().size() == 1);
        assert(nodes_[node->getParents()[0].first].isChild(node->getID()));
        nodes_[node->getParents()[0].first].eraseChild(node->getID());
        assert(!nodes_[node->getParents()[0].first].isChild(node->getID()));
        // We should not add to parent as it will lead to replication of actions
        // as we process the tree in DFS manner
        nodes_[node->getParents()[0].first].addChild(currentNode->getID());
        assert(getInstanceForPath(node->getContext())->getModel() ==
               (getInstanceForPath(currentNode->getContext()))->getModel());
        currentNode->addParent(node->getParents()[0]);
        assert(node->getParents().size() == 1);
        node->eraseParent(node->getParents()[0]);
        assert(node->getParents().size() == 0);
        // FIXME xtof nodes_[node->getID()].eraseParent(node->getParents()[0]);
      }
    }
  }
  bool foundOrphanes = true;
  while (foundOrphanes) {
    // Delete all orphanes nodes and detach them from their children
    // Why? So children will not have fake parents that can lead to isPartOfTree to fail.
    foundOrphanes = false;
    for (auto& node : nodes_) {
      if (node.getParents().size() == 0 && node.getChildren().size() > 0) {
        foundOrphanes = true;
        std::vector<size_t> children = node.getChildren();
        for (auto child : children) {
          for (auto parent : nodes_[child].getParents()) {
            if (parent.first == node.getID()) {
              nodes_[child].eraseParent(parent);
            }
          }
          node.eraseChild(child);
        }
      }
    }
  }
  verifyTree();
}

void ActionTree::verifyTree() const {
  // Verify in dfs manner that all nodes are part of the tree
  std::stack<const ActionTreeNode*> stack;
  stack.push(&nodes_[0]);
  while (!stack.empty()) {
    const ActionTreeNode* currentNode = stack.top();
    stack.pop();
    assert(currentNode->isPartOfTree());
    for (auto& child : currentNode->getChildren()) {
      stack.push(&nodes_[child]);
    }
  }
}

void ActionTree::process() {
  if (!blockNormalization_) {
    normalize();
  }
  // process commands in the bfs manner
  std::queue<ActionTreeNode*> queue;
  queue.push(&nodes_[0]);
  while (!queue.empty()) {
    ActionTreeNode* currentNode = queue.front();
    queue.pop();
    bool allChildrenProcessed = true;
    for (auto& child : currentNode->getChildren()) {
      if (!nodes_[child].isProcessed()) {
        allChildrenProcessed = false;
        queue.push(&nodes_[child]);
      }
    }
    if (!allChildrenProcessed) {
      queue.push(currentNode);
      continue;
    }
    if (currentNode->isProcessed()) {
      continue;
    }
    currentNode->markAsProcessed();
    if (/*!currentNode->getActions().empty() &&*/ currentNode->getParents()[0].first != (size_t)-1
            /*&& !nodes_[currentNode->getParents()[0].first].getContext().empty()*/)
        {
      SNLDesign* top = NLUniverse::get()->getTopDesign();
      SNLDesign* design = top;
      std::vector<NLID::DesignObjectID> path = currentNode->getContext();
      // path.pop_back();
      std::string id("");
      std::vector<NLID::DesignObjectID> pathIds;
      while (!path.empty()) {
        NLID::DesignObjectID name = path.front();
        path.erase(path.begin());
        SNLInstance* inst = design->getInstance(name);
        assert(inst);
        design = inst->getModel();
        id += "_" + std::to_string(design->getID()) + "_" +
              std::to_string(inst->getID());
        pathIds.push_back(inst->getID());
      }
      auto context = currentNode->getContext();
      SNLUniquifier uniquifier(context, id, true);
      uniquifier.process();
    }
    for (auto& action : currentNode->getActions()) {
      if (actions_[action.order]->getContext().empty()) {
        actions_[action.order]->processOnContext(
            NLUniverse::get()->getTopDesign());
      } else {
        actions_[action.order]->processOnContext(
            getInstanceForPath(actions_[action.order]->getContext())
                ->getModel());
      }
    }
    if (currentNode->getParents()[0].first == (size_t)-1) {
      continue;
    }
    // set all paths of parents to the new model
    for (auto& parent : currentNode->getParents()) {
      std::vector<NLID::DesignObjectID> context =
          nodes_[parent.first].getContext(); // <-- the crush
      context.push_back((unsigned)parent.second);
      getInstanceForPath(context)->setModel(
          getInstanceForPath(currentNode->getContext())->getModel());
    }
  }
}
// Destructor that releases all the actions
ActionTree::~ActionTree() {
  for (auto& action : actions_) {
    action->destroy();
  }
}

ActionTreeNode* ActionTreeNode::getChild(NLID::DesignObjectID instance) {
  for (auto& child : children_) {
    if (tree_->getNode(child).getInstance() == instance) {
      return &tree_->getNode(child);
    }
  }
  return nullptr;
}

std::vector<NLID::DesignObjectID> ActionTreeNode::getContext() const {
  std::vector<NLID::DesignObjectID> context;
  // collect the DOID from node until root and then reverse
  const ActionTreeNode* currentNode = this;
  assert(currentNode->isPartOfTree());
  while (currentNode->getParents()[0].first != (size_t)-1) {
    context.push_back(currentNode->getInstance());
    if (currentNode->getParents().empty()) {
      assert(false);
    }
    currentNode = &tree_->getNode(currentNode->getParents().front().first);
  }
  std::reverse(context.begin(), context.end());
  return context;
}

bool ActionTreeNode::operator==(const ActionTreeNode& rhs) const {
  if (actions_.size() != rhs.actions_.size()) {
    return false;
  }
  for (size_t i = 0; i < actions_.size(); i++) {
    if (actions_[i].type != rhs.actions_[i].type) {
      return false;
    }
    if (*(tree_->getAction(actions_[i].order)) !=
        *(tree_->getAction(rhs.actions_[i].order))) {
      return false;
    }
  }
  if (children_.size() == rhs.getChildren().size()) {
    for (size_t i = 0; i < children_.size(); i++) {
      if (tree_->getNode(children_[i]) !=
          tree_->getNode(rhs.getChildren()[i])) {
        return false;
      }
    }
  } else {
    return false;
  }
  // instance_ is not comapred as we are interested in the node model and
  // actions, the actual context will always be different
  return snlid_ == rhs.snlid_;
}

void ActionTreeNode::sortActions() {  // sort actions with custom comparator
                                      // that compare the actuall actions by
                                      // accising th tree
  std::sort(actions_.begin(), actions_.end(),
            [this](const ActionID& lhs, const ActionID& rhs) {
              if (lhs.type != rhs.type) {
                return lhs.type < rhs.type;
              }
              return *(tree_->getAction(lhs.order)) <
                     *(tree_->getAction(rhs.order));
            });
}

bool ActionTreeNode::isPartOfTree() const {
  // Verifying parents lead to root(parent == -1)
  const ActionTreeNode* currentNode = this;
  if (currentNode->getParents().empty()) {
    // If orphan so not part of tree.
    return false;
  }
  while (currentNode->getParents()[0].first != (size_t)-1) {
    if (currentNode->getParents().empty()) {
      return false;
    }
    currentNode = &tree_->getNode(currentNode->getParents().front().first);
  }
  return true;
}
