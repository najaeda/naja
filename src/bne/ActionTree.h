// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <tuple>
#include "SNLID.h"
#include "SNLUniverse.h"
#include "actions.h"
// for find
#include <algorithm>
#include <queue>
#include "Utils.h"
// truth table
#include "SNLDesignTruthTable.h"
#include "SNLLibraryTruthTables.h"
#include "SNLTruthTable.h"

using namespace naja::SNL;

namespace naja::SNL {
class SNLDesign;
class SNLInstance;
}  // namespace naja::SNL

namespace naja::BNE {

class Action;  // Forward declaration of the base class

struct ActionID {
  ActionType type = ActionType::NONE;
  size_t order = 0;
  // compartor
  bool operator==(const ActionID& rhs) const {
    return type == rhs.type && order == rhs.order;
  }
};

class ActionTree;

class ActionTreeNode {
 public:
  ActionTreeNode(SNLID::DesignObjectID instance,
                 SNLID snlid,
                 std::pair<size_t, size_t> parent,
                 size_t id,
                 ActionTree* tree)
      : instance_(instance), snlid_(snlid), id_(id), tree_(tree) {
    parents_.push_back(parent);
  }
  SNLID::DesignObjectID getInstance() const { return instance_; }
  SNLID getSNLID() const { return snlid_; }
  void addAction(const ActionID& action) { actions_.push_back(action); }
  void addParent(std::pair<size_t, size_t> parent) {
    parents_.push_back(parent);
  }
  void eraseChild(size_t id) {
    children_.erase(std::find(children_.begin(), children_.end(), id));
  }
  void addChild(size_t id) { children_.push_back(id); }
  ActionTreeNode* getChild(SNLID::DesignObjectID instance);
  const std::vector<ActionID>& getActions() const { return actions_; }
  const std::vector<size_t>& getChildren() const { return children_; }
  void addActions(const std::vector<ActionID>& actions) {
    actions_.insert(actions_.end(), actions.begin(), actions.end());
  }
  // compartor
  bool operator==(const ActionTreeNode& rhs) const;
  const std::vector<std::pair<size_t, size_t>>& getParents() const {
    return parents_;
  }
  size_t getID() const { return id_; }
  void eraseParent(const std::pair<size_t, size_t>& id) {
    parents_.erase(std::find(parents_.begin(), parents_.end(), id));
  }
  std::vector<SNLID::DesignObjectID> getContext() const;
  bool isProcessed() const { return processed_; }
  void markAsProcessed() { processed_ = true; }
  void sortActions();

 private:
  std::vector<ActionID> actions_;
  SNLID::DesignObjectID instance_;
  SNLID snlid_;
  std::vector<size_t> children_;
  std::vector<std::pair<size_t, size_t>> parents_;
  size_t id_ = 0;
  ActionTree* tree_ = nullptr;
  bool processed_ = false;
};

class ActionTree {
 public:
  ActionTree(bool blockNormalization = false, bool keepOrder = false);
  ActionTreeNode* getNodeForContext(
      const std::vector<SNLID::DesignObjectID>& context);
  ActionTreeNode& addHierChild(
      const std::vector<SNLID::DesignObjectID>& context);
  void addAction(Action* action);
  void normalize();
  ActionTreeNode& getRoot() { return nodes_[0]; }
  ActionTreeNode& getNode(size_t id) { return nodes_[id]; }
  void process();
  Action* getAction(size_t id) { return actions_[id]; }
  // Destructor that releases all the actions
  ~ActionTree();

 private:
  std::vector<ActionTreeNode> nodes_;
  std::vector<Action*> actions_;
  bool blockNormalization_ = false;
  bool keepOrder_ = false;
};

}  // namespace naja::BNE