// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "bne.h"
#include "SNLDesignObject.h"
#include "SNLDesign.h"
#include "SNLUniverse.h"
#include "SNLInstance.h"
#include "Utils.h"
#include "SNLID.h"

// #define ASSERT_NORMALIZATION_PER_PATH

class Action; // Forward declaration of the base class

ActionTreeNode *ActionTreeNode::getChild(SNLID::DesignObjectID instance)
{
    // printf("getChild instance %u\n", instance);
    for (auto &child : children_)
    {
        if (tree_->getNode(child).getInstance() == instance)
        {
            // printf("found child %u\n", instance);
            return &tree_->getNode(child);
        }
    }
    return nullptr;
}

std::vector<SNLID::DesignObjectID> ActionTreeNode::getContext() const
{
    std::vector<SNLID::DesignObjectID> context;
    // collect the DOID from node until root and then reverse
    const ActionTreeNode *currentNode = this;
    while (currentNode->getParents()[0].first != (size_t)-1)
    {
        context.push_back(currentNode->getInstance());
        if (currentNode->getParents().empty())
        {
            break;
        }
        currentNode = &tree_->getNode(currentNode->getParents().front().first);
    }
    std::reverse(context.begin(), context.end());
    return context;
}
void ActionTree::addDeleteAction(const std::vector<SNLID::DesignObjectID> &pathToDelete)
{
    // printf("here\n");
    deleteActions_.push_back(DeleteAction(pathToDelete));
    actions_.push_back({ActionType::DELETE, deleteActions_.size() - 1}); // Cast to the base class
    if (deleteActions_.back().getContext().size() > 0)
    {
        addHierChild(deleteActions_.back().getContext());
    }
    std::vector<ActionID> actions;
    actions.push_back(actions_.back());
    getNodeForContext(deleteActions_.back().getContext())->addActions(actions);
}

bool ActionTreeNode::operator==(const ActionTreeNode &rhs) const
{
    if (actions_.size() != rhs.actions_.size())
    {
        return false;
    }
    for (size_t i = 0; i < actions_.size(); i++)
    {
        if (actions_[i].type != rhs.actions_[i].type)
        {
            return false;
        }
        switch (actions_[i].type)
        {
        case ActionType::DELETE:
            if (tree_->getDeleteActions()[actions_[i].order] != tree_->getDeleteActions()[rhs.actions_[i].order])
            {
                return false;
            }
            break;
        case ActionType::DRIVE_WITH_CONSTANT:
            if (tree_->getDriveWithConstantActions()[actions_[i].order] != tree_->getDriveWithConstantActions()[rhs.actions_[i].order])
            {
                return false;
            }
            break;
        case ActionType::REDUCTION:
            if (tree_->getReductionActions()[actions_[i].order] != tree_->getReductionActions()[rhs.actions_[i].order])
            {
                return false;
            }
            break;
        default:
            break;
        }
    }
    if (children_.size() == rhs.getChildren().size())
    {
        for (size_t i = 0; i < children_.size(); i++)
        {
            if (tree_->getNode(children_[i]) != tree_->getNode(rhs.getChildren()[i]))
            {
                return false;
            }
        }
    } else {
        return false;
    }
    return instance_ == rhs.instance_ && snlid_ == rhs.snlid_;
}

void ActionTreeNode::sortActions() { //sort actions with custom comparator that compare the actuall actions by accising th tree
        std::sort(actions_.begin(), actions_.end(), [this](const ActionID &lhs, const ActionID &rhs) {
            if (lhs.type != rhs.type)
            {
                return lhs.type < rhs.type;
            }
            switch (lhs.type)
            {
            case ActionType::DELETE:
                return tree_->getDeleteActions()[lhs.order] < tree_->getDeleteActions()[rhs.order];
            case ActionType::DRIVE_WITH_CONSTANT:
                return tree_->getDriveWithConstantActions()[lhs.order] < tree_->getDriveWithConstantActions()[rhs.order];
            case ActionType::REDUCTION:
                return tree_->getReductionActions()[lhs.order] < tree_->getReductionActions()[rhs.order];
            default:
                return false;
            }
        });
    }