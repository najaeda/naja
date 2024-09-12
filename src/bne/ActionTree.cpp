// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "ActionTree.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"
#include "SNLID.h"
#include "SNLInstance.h"
#include "SNLUniverse.h"
#include "Utils.h"

using namespace naja::SNL;
using namespace naja::BNE;

ActionTree::ActionTree(bool keepOrder, bool blockNormalization)
    : blockNormalization_(blockNormalization), keepOrder_(keepOrder) {
  nodes_.push_back({0, SNLUniverse::get()->getTopDesign()->getSNLID(),
                    std::pair<size_t, size_t>((size_t)-1, 0), 0, this});
}
ActionTreeNode* ActionTree::getNodeForContext(
    const std::vector<SNLID::DesignObjectID>& context) {
  size_t id = nodes_[0].getID();
  for (auto& instance : context) {
    // printf("instance %u\n", instance);
    ActionTreeNode* child = nodes_[id].getChild(instance);
    if (child == nullptr) {
      return nullptr;
    }
    id = child->getID();
  }
  return &nodes_[id];
}
ActionTreeNode& ActionTree::addHierChild(
    const std::vector<SNLID::DesignObjectID>& context) {
  size_t id = nodes_[0].getID();
  SNLDesign* model = SNLUniverse::get()->getTopDesign();
  std::vector<SNLID::DesignObjectID> partialContextCheck;
  for (auto& instance : context) {
    partialContextCheck.push_back(instance);
    model = model->getInstance(instance)->getModel();
    ActionTreeNode* child = nodes_[id].getChild(instance);
    if (child == nullptr) {
      nodes_.push_back({instance, model->getSNLID(),
                        std::pair<size_t, size_t>(nodes_[id].getID(), instance),
                        nodes_.size(), this});
      // printf("Adding node %zu\n", nodes_.size() - 1);
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
  std::map<SNLID, std::vector<ActionTreeNode*>> snlid2Node;
  std::vector<ActionTreeNode*> queue;
  queue.push_back(&nodes_[0]);
  while (!queue.empty()) {
    ActionTreeNode* currentNode = queue.back();
    queue.pop_back();
    if (snlid2Node.find(currentNode->getSNLID()) != snlid2Node.end()) {
      assert(getInstanceForPath(
                 snlid2Node[currentNode->getSNLID()].back()->getContext())
                 ->getModel() ==
             (getInstanceForPath(currentNode->getContext()))->getModel());
    }
    snlid2Node[currentNode->getSNLID()].push_back(currentNode);

    for (auto& child : currentNode->getChildren()) {
      queue.push_back(&nodes_[child]);
    }
  }
  for (auto& it : snlid2Node) {
    auto nodes = it.second;
    // If top, continue
    if (nodes[0]->getParents()[0].first == (size_t)-1) {
      continue;
    }
    bool foundNormalization = true;
    while (foundNormalization && nodes.size() > 1) {
      foundNormalization = false;
      std::vector<ActionTreeNode*> nodesToMerge;
      auto currentNode = nodes.back();
      nodesToMerge.push_back(currentNode);
      nodes.pop_back();
      for (auto node : nodes) {
        if (*currentNode == *node) {
          nodesToMerge.push_back(node);
          foundNormalization = true;
        }
      }
      /*if (nodesToMerge.size() > 1) {
        //printf("Merging nodes %lu nodes\n", nodesToMerge.size());
        for (auto node : nodesToMerge) {
          auto design = SNLUniverse::get()->getTopDesign();
          // Print the context names
          for (auto& instance : node->getContext()) {
            printf(
                "%s ",
                design->getInstance(instance)->getName().getString().c_str());
            design = design->getInstance(instance)->getModel();
          }
          //printf("\n");
        }
      }*/
      for (auto node : nodesToMerge) {
        if (node == currentNode) {
          continue;
        }
        nodes.erase(std::find(nodes.begin(), nodes.end(), node));
        assert(node->getParents().size() == 1);
        nodes_[node->getParents()[0].first].eraseChild(node->getID());
        // We should not add to parent as it will lead to replication of actions
        // as we process the tree in DFS manner
        nodes_[node->getParents()[0].first].addChild(currentNode->getID());
        assert(getInstanceForPath(node->getContext())->getModel() ==
               (getInstanceForPath(currentNode->getContext()))->getModel());
        currentNode->addParent(node->getParents()[0]);
        nodes_[node->getID()].eraseParent(node->getParents()[0]);
      }
    }
  }
  bool foundOrphanes = true;
  while (foundOrphanes) {
    // Delete all orphanes nodes and detach them from their children
    foundOrphanes = false;
    for (auto& node : nodes_) {
      if (node.getParents().size() == 0 && node.getChildren().size() > 0) {
        foundOrphanes = true;
        for (auto child : node.getChildren()) {
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
}

void ActionTree::process() {
  // printf("Processing actions\n");
  if (!blockNormalization_) {
    normalize();
  }
  // process commands in the dfs manner
  std::queue<ActionTreeNode*> queue;
  queue.push(&nodes_[0]);
  while (!queue.empty()) {
    // printf("--------------------------------new
    // node------------------------------------\n");
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
      SNLDesign* top = SNLUniverse::get()->getTopDesign();
      SNLDesign* design = top;
      std::vector<SNLID::DesignObjectID> path = currentNode->getContext();
      // path.pop_back();
      std::string id("");
      std::vector<SNLID::DesignObjectID> pathIds;
      while (!path.empty()) {
        SNLID::DesignObjectID name = path.front();
        path.erase(path.begin());
        SNLInstance* inst = design->getInstance(name);
        assert(inst);
        design = inst->getModel();
        // printf("%s ", inst->getName().getString().c_str());
        id += "_" + std::to_string(design->getID()) + "_" +
              std::to_string(inst->getID());
        pathIds.push_back(inst->getID());
      }
      auto context = currentNode->getContext();
      Uniquifier uniquifier(context, id, true);
      uniquifier.process();
    }
    for (auto& action : currentNode->getActions()) {
      // printf("--------------------------------processing
      // actions------------------------------------\n");
      if (actions_[action.order]->getContext().empty()) {
        actions_[action.order]->processOnContext(
            SNLUniverse::get()->getTopDesign());
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
      std::vector<SNLID::DesignObjectID> context =
          nodes_[parent.first].getContext();
      context.push_back((unsigned)parent.second);
      // printf("--------------------------------setting
      // parent------------------------------------\n");
      getInstanceForPath(context)->setModel(
          getInstanceForPath(currentNode->getContext())->getModel());
    }
  }
}
// Destructor that releases all the actions
ActionTree::~ActionTree() {
  for (auto& action : actions_) {
    switch (action->getType()) {
      case ActionType::DELETE:
        delete static_cast<DeleteAction*>(action);
        break;
      case ActionType::DRIVE_WITH_CONSTANT:
        delete static_cast<DriveWithConstantAction*>(action);
        break;
      case ActionType::REDUCTION:
        delete static_cast<ReductionAction*>(action);
        break;
      default:
        break;
    }
  }
}

ActionTreeNode* ActionTreeNode::getChild(SNLID::DesignObjectID instance) {
  // printf("getChild instance %u\n", instance);
  for (auto& child : children_) {
    if (tree_->getNode(child).getInstance() == instance) {
      // printf("found child %u\n", instance);
      return &tree_->getNode(child);
    }
  }
  return nullptr;
}

std::vector<SNLID::DesignObjectID> ActionTreeNode::getContext() const {
  std::vector<SNLID::DesignObjectID> context;
  // collect the DOID from node until root and then reverse
  const ActionTreeNode* currentNode = this;
  while (currentNode->getParents()[0].first != (size_t)-1) {
    context.push_back(currentNode->getInstance());
    if (currentNode->getParents().empty()) {
      break;
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
  //instance_ is not comapred as we are interested in the node model and actions, the actual context will always be different
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