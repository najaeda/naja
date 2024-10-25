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
  /**
   * \brief Create an ActionTreeNode.
   * \param instance the instance of the node.
   * \param snlid the snlid of the node.
   * \param parent the parent of the node.
   * \param id the id of the node.
   * \param tree the tree of the node.
   * \return the created ActionTreeNode.
   */
  ActionTreeNode(SNLID::DesignObjectID instance,
                 SNLID snlid,
                 std::pair<size_t, size_t> parent,
                 size_t id,
                 ActionTree* tree)
      : instance_(instance), snlid_(snlid), id_(id), tree_(tree) {
    parents_.push_back(parent);
  }
  /**
   * \brief get the instance of the node.
   * \return the node's instance object id.
   */
  SNLID::DesignObjectID getInstance() const { return instance_; }
  /**
   * \brief get the snlid of the node.
   * \return the snlid of the node.
   */
  SNLID getSNLID() const { return snlid_; }
  /**
   * \brief add an action to the node.
   * \param action the action to add.
   */
  void addAction(const ActionID& action) { actions_.push_back(action); }
  /**
   * \brief add a parent to the node.
   * \param parent the parent to add.
   */
  void addParent(std::pair<size_t, size_t> parent) {
    parents_.push_back(parent);
  }
  /**
   * \brief erase a child from the node.
   * \param id the id of the child to erase.
   */
  void eraseChild(size_t id) {
    children_.erase(std::find(children_.begin(), children_.end(), id));
  }
  /**
   * \brief add a child to the node.
   * \param id the id of the child to add.
   */
  void addChild(size_t id) { children_.push_back(id); }
  /**
   * \brief get the child of the node.
   * \param instance the instance of the child.
   * \return the child of the node.
   */
  ActionTreeNode* getChild(SNLID::DesignObjectID instance);
  /**
   * \brief get the children of the node.
   * \return the children of the node.
   */
  const std::vector<ActionID>& getActions() const { return actions_; }
  /**
   * \brief get the children of the node.
   * \return the children of the node.
   */
  const std::vector<size_t>& getChildren() const { return children_; }
  /**
   * \brief add actions to the node.
   * \param actions the actions to add.
   */
  void addActions(const std::vector<ActionID>& actions) {
    actions_.insert(actions_.end(), actions.begin(), actions.end());
  }
  // compartor
  bool operator==(const ActionTreeNode& rhs) const;
  const std::vector<std::pair<size_t, size_t>>& getParents() const {
    return parents_;
  }
  /**
   * \brief get the id of the node.
   * \return the id of the node.
   */
  size_t getID() const { return id_; }
  /**
   * \brief erase a parent from the node.
   * \param id the id of the parent to erase.
   */
  void eraseParent(const std::pair<size_t, size_t>& id) {
    parents_.erase(std::find(parents_.begin(), parents_.end(), id));
  }
  /**
   * \brief get the context of the node.
   * \return the context of the node.
   */
  std::vector<SNLID::DesignObjectID> getContext() const;
  /**
   * \brief get the tree of the node.
   * \return the tree of the node.
   */
  bool isProcessed() const { return processed_; }
  /**
   * \brief mark the node as processed.
   */
  void markAsProcessed() { processed_ = true; }
  /**
   * \brief sort the actions of the node. 
   */
  void sortActions();
  /**
   * \brief get the tree of the node.
   * \return the tree of the node.
   */
  bool isPartOfTree() const;
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
  /**
   * \brief Create an ActionTree.
   * \param blockNormalization true if the normalization is blocked.
   * \param keepOrder true if the order is kept.
   * \return the created ActionTree.
   */
  ActionTree(bool blockNormalization = false, bool keepOrder = false);
  /**
   * \brief get the node for the context.
   * \param context the context of the node.
   * \return the node for the context.
   */
  ActionTreeNode* getNodeForContext(
      const std::vector<SNLID::DesignObjectID>& context);
  /**
   * \brief add a node childe for the context path.
   * \param context the context of the node.
   * \return the node for the context.
   */
  ActionTreeNode& addHierChild(
      const std::vector<SNLID::DesignObjectID>& context);
  /**
   * \brief add a node childe for the context path.
   * \param context the context of the node.
   * \return the node for the context.
   */
  void addAction(Action* action);
  /**
   * \brief normalize the tree.
   */
  void normalize();
  /**
   * \brief get the root of the tree.
   * \return the root of the tree.
   */
  ActionTreeNode& getRoot() { return nodes_[0]; }
  /**
   * \brief get the node of the id.
   * \param id the id of the node.
   * \return the node of the id.
   */
  ActionTreeNode& getNode(size_t id) { return nodes_[id]; }
  /**
   * \brief process the tree.
   */
  void process();
  /**
   * \brief get the action of the id.
   * \param id the id of the action.
   * \return the action of the id.
   */
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