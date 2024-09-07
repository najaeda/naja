// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <tuple>
#include "actions.h"
#include "SNLID.h"
#include "SNLUniverse.h"
//for find
#include <algorithm>
#include <queue>
#include "Utils.h"
//truth table
#include "SNLDesignTruthTable.h"
#include "SNLTruthTable.h"
#include "SNLLibraryTruthTables.h"

using namespace naja::SNL;
using namespace naja::NAJA_OPT;

namespace naja::SNL
{
    class SNLDesign;
    class SNLInstance;
}

class Action; // Forward declaration of the base class

struct ActionID
{
    ActionType type = ActionType::NONE;
    size_t order = 0;
    //compartor
    bool operator==(const ActionID &rhs) const
    {
        return type == rhs.type && order == rhs.order;
    }
};

class ActionTree;

class ActionTreeNode
{
public:
    ActionTreeNode(SNLID::DesignObjectID instance, SNLID snlid, std::pair<size_t, size_t> parent, size_t id, ActionTree *tree) : 
        instance_(instance), snlid_(snlid), id_(id), tree_(tree) { parents_.push_back(parent); }
    SNLID::DesignObjectID getInstance() const { return instance_; }
    SNLID getSNLID() const { return snlid_; }
    void addAction(const ActionID &action) { actions_.push_back(action); }
    void addParent(std::pair<size_t, size_t> parent) { parents_.push_back(parent); }
    void eraseChild(size_t id)
    {
        children_.erase(std::find(children_.begin(), children_.end(), id));
    }
    void addChild(size_t id) { children_.push_back(id); }
    ActionTreeNode *getChild(SNLID::DesignObjectID instance);
    const std::vector<ActionID> &getActions() const { return actions_; }
    const std::vector<size_t> &getChildren() const { return children_; }
    void addActions(const std::vector<ActionID> &actions) { actions_.insert(actions_.end(), actions.begin(), actions.end()); }
    //compartor
    bool operator==(const ActionTreeNode &rhs) const;
    const std::vector<std::pair<size_t, size_t>>& getParents() const { return parents_; }
    size_t getID() const { return id_; }
    void eraseParent(const std::pair<size_t, size_t>& id)
    {
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

class ActionTree
{
public:
    ActionTree() { 
        nodes_.push_back({0, SNLUniverse::get()->getTopDesign()->getSNLID(), std::pair<size_t, size_t>((size_t) -1, 0), 0, this}); 
    }
    ActionTreeNode* getNodeForContext(const std::vector<SNLID::DesignObjectID> &context)
    {
        size_t id = nodes_[0].getID();
        for (auto &instance : context)
        {
            //printf("instance %u\n", instance);
            ActionTreeNode *child = nodes_[id].getChild(instance);
            if (child == nullptr)
            {
                return nullptr;
            }
            id = child->getID();
        }
        return &nodes_[id];
    }
    ActionTreeNode& addHierChild(const std::vector<SNLID::DesignObjectID> &context)
    {
        size_t id = nodes_[0].getID();
        SNLDesign *model = SNLUniverse::get()->getTopDesign();
        std::vector<SNLID::DesignObjectID> partialContextCheck;
        for (auto &instance : context)
        {
            partialContextCheck.push_back(instance);
            model = model->getInstance(instance)->getModel();
            ActionTreeNode *child = nodes_[id].getChild(instance);
            if (child == nullptr)
            {
                nodes_.push_back({instance, model->getSNLID(), std::pair<size_t,size_t>(nodes_[id].getID(), instance), nodes_.size(), this});
                //printf("Adding node %zu\n", nodes_.size() - 1);
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
    void addDeleteAction(const std::vector<SNLID::DesignObjectID> &pathToDelete);

    void addDriveWithConstantAction(const std::vector<SNLID::DesignObjectID> &context,
                                    const SNLID::DesignObjectID &pathToDrive, const SNLID::DesignObjectID &termToDrive, 
                                    const double &value, SNLBitTerm *topTermToDrive = nullptr)
    {
        driveWithConstantActions_.push_back(DriveWithConstantAction(context, pathToDrive, termToDrive, value, topTermToDrive));
        actions_.push_back({ActionType::DRIVE_WITH_CONSTANT, driveWithConstantActions_.size() - 1});
        if (driveWithConstantActions_.back().getContext().size() > 0) {
            addHierChild(driveWithConstantActions_.back().getContext());
        }
        std::vector<ActionID> actions;
        actions.push_back(actions_.back());
        //Add action to node
        getNodeForContext(driveWithConstantActions_.back().getContext())->addActions(actions);
    }
    void addReductionCommand(const std::vector<SNLID::DesignObjectID> &context, SNLID::DesignObjectID instance,
    const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result)
    {
        reductionActions_.push_back(ReductionAction(context, instance, result));
        actions_.push_back({ActionType::REDUCTION, reductionActions_.size() - 1});
        if (reductionActions_.back().getContext().size() > 0) {
            addHierChild(reductionActions_.back().getContext());
        }
        std::vector<ActionID> actions;
        actions.push_back(actions_.back());
        //Add action to node
        getNodeForContext(reductionActions_.back().getContext())->addActions(actions);
    }
    void normalize() {
        //Sort actions on all nodes
        if (!keepOrder_) {
            for (auto &node : nodes_)
            {
                node.sortActions();
            }
        }
        //Collect in dfs manner all the tree nodes into a map with key of snlid
        std::map<SNLID, std::vector<ActionTreeNode*>> snlid2Node;
        std::vector<ActionTreeNode*> queue;
        queue.push_back(&nodes_[0]);
        while (!queue.empty())
        {
            ActionTreeNode *currentNode = queue.back();
            queue.pop_back();
            if (snlid2Node.find(currentNode->getSNLID()) != snlid2Node.end())
            {
                assert(getInstanceForPath(snlid2Node[currentNode->getSNLID()].back()->getContext())->getModel()
                == (getInstanceForPath(currentNode->getContext()))->getModel());
            }
            snlid2Node[currentNode->getSNLID()].push_back(currentNode);

            for (auto &child : currentNode->getChildren())
            {
                queue.push_back(&nodes_[child]);
            }
        }
        for (auto &it : snlid2Node)
        {
            auto nodes = it.second;
            //If top, continue
            if (nodes[0]->getParents()[0].first == (size_t) -1)
            {
                continue;
            }
            bool foundNormalization = true;
            while (foundNormalization && nodes.size() > 1)
            {
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
                if (nodesToMerge.size() > 1) {
                    printf("Merging nodes %lu nodes\n", nodesToMerge.size());
                    for (auto node : nodesToMerge) {
                        auto design = SNLUniverse::get()->getTopDesign();
                        //Print the context names
                        for (auto &instance : node->getContext())
                        {
                            printf("%s ", design->getInstance(instance)->getName().getString().c_str());
                            design = design->getInstance(instance)->getModel();
                        }
                        printf("\n");
                    }
                }
                for (auto node : nodesToMerge) {
                    if (node == currentNode) {
                        continue;
                    }
                    nodes.erase(std::find(nodes.begin(), nodes.end(), node));
                    assert(node->getParents().size() == 1);
                    nodes_[node->getParents()[0].first].eraseChild(node->getID());
                    //We should not add to parent as it will lead to replication of actions
                    //as we process the tree in DFS manner
                    nodes_[node->getParents()[0].first].addChild(currentNode->getID());
                    assert(getInstanceForPath(node->getContext())->getModel()
                == (getInstanceForPath(currentNode->getContext()))->getModel());
                    currentNode->addParent(node->getParents()[0]);
                    nodes_[node->getID()].eraseParent(node->getParents()[0]);
                }
            }
        }
        bool foundOrphanes = true;
        while (foundOrphanes) {
            //Delete all orphanes nodes and detach them from their children
            foundOrphanes = false;
            for (auto& node : nodes_)
            {
                if (node.getParents().size() == 0 && node.getChildren().size() > 0)
                {
                    foundOrphanes = true;
                    for (auto child : node.getChildren())
                    {
                        for (auto parent : nodes_[child].getParents())
                        {
                            if (parent.first == node.getID())
                            {
                                nodes_[child].eraseParent(parent);
                            }
                        }
                        node.eraseChild(child);
                    }
                }
            }
        }
    }
    ActionTreeNode& getRoot() { return nodes_[0]; }
    ActionTreeNode& getNode(size_t id) { return nodes_[id]; }
    void process() {
        //printf("Processing actions\n");
        if (!blockNormalization_) {
            normalize();
        }
        //process commands in the dfs manner
        std::queue<ActionTreeNode*> queue;
        queue.push(&nodes_[0]);
        while (!queue.empty())
        {
            //printf("--------------------------------new node------------------------------------\n");
            ActionTreeNode *currentNode = queue.front();
            queue.pop();
            bool allChildrenProcessed = true;
            for (auto &child : currentNode->getChildren())
            {
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
            if (/*!currentNode->getActions().empty() &&*/ currentNode->getParents()[0].first != (size_t) -1
                /*&& !nodes_[currentNode->getParents()[0].first].getContext().empty()*/) {
                SNLDesign *top = SNLUniverse::get()->getTopDesign(); 
                SNLDesign *design = top;
                std::vector<SNLID::DesignObjectID> path = currentNode->getContext();
                // path.pop_back();
                std::string id("");
                std::vector<SNLID::DesignObjectID> pathIds;
                while (!path.empty())
                {
                    SNLID::DesignObjectID name = path.front();
                    path.erase(path.begin());
                    SNLInstance *inst = design->getInstance(name);
                    assert(inst);
                    design = inst->getModel();
                    //printf("%s ", inst->getName().getString().c_str());
                    id += "_" + std::to_string(design->getID()) + "_" + std::to_string(inst->getID());
                    pathIds.push_back(inst->getID());
                }
                auto context = currentNode->getContext();
                naja::NAJA_OPT::Uniquifier uniquifier(context, id, true);
                uniquifier.process();
            }
            for (auto &action : currentNode->getActions())
            {
                //printf("--------------------------------processing actions------------------------------------\n");
                switch (action.type)
                {
                case ActionType::DELETE:
                {
                    DeleteAction &deleteAction = deleteActions_[action.order];
                    if (deleteAction.getContext().empty()) {
                        deleteAction.processOnContext(SNLUniverse::get()->getTopDesign());
                    } else {
                        deleteAction.processOnContext(getInstanceForPath(deleteAction.getContext())->getModel());
                    }
                    break;
                }
                case ActionType::DRIVE_WITH_CONSTANT:
                {
                    DriveWithConstantAction &driveWithConstantAction = driveWithConstantActions_[action.order];
                    if (driveWithConstantAction.getContext().empty()) {
                        driveWithConstantAction.processOnContext(SNLUniverse::get()->getTopDesign());
                    } else {
                        driveWithConstantAction.processOnContext(getInstanceForPath(driveWithConstantAction.getContext())->getModel());
                    }
                    break;
                }
                case ActionType::REDUCTION:
                {
                    ReductionAction &reductionAction = reductionActions_[action.order];
                    if (reductionAction.getContext().empty()) {
                        reductionAction.processOnContext(SNLUniverse::get()->getTopDesign());
                    } else {
                        reductionAction.processOnContext(getInstanceForPath(reductionAction.getContext())->getModel());
                    }
                    break;
                }
                default:
                    break;
                }
            }
            if (currentNode->getParents()[0].first == (size_t) -1)
            {
                continue;
            }
            //set all paths of parents to the new model
            for (auto &parent : currentNode->getParents())
            {
                std::vector<SNLID::DesignObjectID> context = nodes_[parent.first].getContext();
                context.push_back((unsigned) parent.second);
                //printf("--------------------------------setting parent------------------------------------\n");
                getInstanceForPath(context)->setModel(getInstanceForPath(currentNode->getContext())->getModel());
            }
        }
    }
    //Get delete actions
    const std::vector<DeleteAction>& getDeleteActions() const { return deleteActions_; }
    //Get drive with constant actions
    const std::vector<DriveWithConstantAction>& getDriveWithConstantActions() const { return driveWithConstantActions_; }
    //Get reduction actions
    const std::vector<ReductionAction>& getReductionActions() const { return reductionActions_; }
private:
    std::vector<ActionTreeNode> nodes_;
    std::vector<ActionID> actions_;
    std::vector<DeleteAction> deleteActions_;
    std::vector<DriveWithConstantAction> driveWithConstantActions_;
    std::vector<ReductionAction> reductionActions_;
    bool blockNormalization_ = false;
    bool keepOrder_ = false;
};