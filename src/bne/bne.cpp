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
    //printf("getChild instance %u\n", instance);
    for (auto &child : children_)
    {
        if (tree_->getNode(child).getInstance() == instance)
        {
            //printf("found child %u\n", instance);
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
    //printf("here\n");
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
            default:
                break;
            }
        }
        if (children_.size() == rhs.getChildren().size()) {
            for (size_t i = 0; i < children_.size(); i++) {
                if (tree_->getNode(children_[i]) != tree_->getNode(rhs.getChildren()[i])) {
                    return false;
                }
            }
        } {
            return false;
        }
        return instance_ == rhs.instance_ && snlid_ == rhs.snlid_; 
    }


void BNE::addDeleteAction(const std::vector<SNLID::DesignObjectID> &pathToDelete)
{
    deleteActions_.push_back(DeleteAction(pathToDelete));
    actions_.push_back({ActionType::DELETE, deleteActions_.size() - 1}); // Cast to the base class
    SNLID::DesignObjectID dbID = 0;
    SNLID::DesignObjectID libraryID = 0;
    SNLID::DesignObjectID designID = 0;
    SNLDesign *design = nullptr;
    if (deleteActions_.back().getContext().empty())
    {
        design = SNLUniverse::get()->getTopDesign();
    }
    else
    {
        design = getInstanceForPath(deleteActions_.back().getContext())->getModel();
    }
    assert(!design->getInstances().empty());
    dbID = design->getSNLID().dbID_;
    libraryID = design->getSNLID().libraryID_;
    designID = design->getSNLID().designID_;
    std::vector<ActionID> actions;
    actions.push_back(actions_.back());
    Doid2ActionsPerPath_[std::tuple<SNLID::DesignObjectID,
                                    SNLID::DesignObjectID,
                                    SNLID::DesignObjectID>(dbID, libraryID, designID)]
        .push_back({actions, deleteActions_.back().getContext()});
}

void BNE::addDriveWithConstantAction(const std::vector<SNLID::DesignObjectID> &context,
                                     const SNLID::DesignObjectID &pathToDrive, const SNLID::DesignObjectID &termToDrive, const double &value, SNLBitTerm *topTermToDrive)
{
    driveWithConstantActions_.push_back(DriveWithConstantAction(context, pathToDrive, termToDrive, value, topTermToDrive));
    actions_.push_back({ActionType::DRIVE_WITH_CONSTANT, driveWithConstantActions_.size() - 1});
    SNLID::DesignObjectID dbID = 0;
    SNLID::DesignObjectID libraryID = 0;
    SNLID::DesignObjectID designID = 0;
    SNLDesign *design = nullptr;
    if (context.empty())
    {
        design = SNLUniverse::get()->getTopDesign();
    }
    else
    {
        design = getInstanceForPath(context)->getModel();
    }
    assert(!design->getInstances().empty());
    dbID = design->getSNLID().dbID_;
    libraryID = design->getSNLID().libraryID_;
    designID = design->getSNLID().designID_;
    std::vector<ActionID> actions;
    actions.push_back(actions_.back());
    Doid2ActionsPerPath_[std::tuple<SNLID::DesignObjectID,
                                    SNLID::DesignObjectID,
                                    SNLID::DesignObjectID>(dbID, libraryID, designID)]
        .push_back({actions, context});
}

void BNE::compressPerPath()
{
    for (auto &it : Doid2ActionsPerPath_)
    {
        auto &actionsPerPath = it.second;
        for (size_t index = 0; index < actionsPerPath.size(); index++)
        {
            auto &action = actionsPerPath[index];
            if (action.actions.empty())
            {
                continue;
            }
            for (size_t candidateIndexForCompression = index + 1; candidateIndexForCompression < actionsPerPath.size(); candidateIndexForCompression++)
            {
                auto &candidateActionForCompression = actionsPerPath[candidateIndexForCompression];
                auto &path = action.path;
                auto &path2 = candidateActionForCompression.path;
                if (path == path2 && !blockNormalization_)
                {
                    action.actions.insert(action.actions.end(), candidateActionForCompression.actions.begin(), candidateActionForCompression.actions.end());
                    candidateActionForCompression.actions.clear();
                    candidateActionForCompression.path.clear();
                }
            }
        }
    }
#ifdef ASSERT_NORMALIZATION_PER_PATH
    for (auto &it : Doid2ActionsPerPath_)
    {
        auto &actionsPerPath = it.second;
        for (size_t index = 0; index < actionsPerPath.size(); index++)
        {
            auto &action = actionsPerPath[index];
            if (action.actions.empty())
            {
                continue;
            }
            for (size_t candidateIndexForCompression = index + 1; candidateIndexForCompression < actionsPerPath.size(); candidateIndexForCompression++)
            {
                auto &candidateActionForCompression = actionsPerPath[candidateIndexForCompression];
                if (candidateActionForCompression.actions.empty())
                {
                    continue;
                }
                auto &path = action.path;
                auto &path2 = candidateActionForCompression.path;
                if (path == path2 && !blockNormalization_)
                {
                    assert(false);
                }
            }
        }
    }
#endif
}
void BNE::normalizeActions()
{
    // Merge similar actions on same path
    compressPerPath();
    // Merge similar actions on same design(not path)
    // Why it comes after path normalization?
    // Because full operation(collection of actions) per path is know only after path normalization
    // and only then normalization per design is possible. Only operations can be merged by design,not scattered actions.
    for (auto &it : Doid2ActionsPerPath_)
    {
        auto actionsPerPath = it.second;
        while (!actionsPerPath.empty())
        {
            NormalizedAction normalizeActions;
            normalizeActions.newDesign = nullptr;
            normalizeActions.paths.push_back(actionsPerPath.back().path);
            normalizeActions.actions = actionsPerPath.back().actions;
            actionsPerPath.pop_back();
            if (normalizeActions.actions.empty())
            {
                continue;
            }
            for (auto &candidateActionForCompression : actionsPerPath)
            {
                if (compareActionsVectorsUnderSameContext(normalizeActions.actions, candidateActionForCompression.actions) && !blockNormalization_) // create a comparator
                {
                    candidateActionForCompression.actions.clear();
                    normalizeActions.paths.push_back(candidateActionForCompression.path);
                }
            }
            Doid2normalizedAction_[it.first].push_back(normalizeActions);
        }
    }
    Doid2ActionsPerPath_.clear();
}

void BNE::processNormalizedActions()
{
    for (auto &it : Doid2normalizedAction_)
    {
        auto &normalizedActions = it.second;
        for (auto &normalizedAction : normalizedActions)
        {
            SNLDesign *top = SNLUniverse::get()->getTopDesign();
            SNLDesign *design = top;
            std::vector<SNLID::DesignObjectID> path =
                normalizedAction.paths.front();
            std::string id("");
            std::vector<SNLID::DesignObjectID> pathIds;
            while (!path.empty())
            {
                SNLID::DesignObjectID name = path.front();
                path.erase(path.begin());
                SNLInstance *inst = design->getInstance(name);
                assert(inst);
                design = inst->getModel();
                id += "_" + std::to_string(design->getID()) + "_" + std::to_string(inst->getID());
                pathIds.push_back(inst->getID());
            }
            if (!normalizedAction.paths.front().empty())
            {
                naja::NAJA_OPT::Uniquifier uniquifier(pathIds, id, true);
                uniquifier.process();
                design = uniquifier.getPathUniq().back()->getModel();
            }
            for (auto &action : normalizedAction.actions)
            {
                switch (action.type)
                {
                case ActionType::DELETE:
                {
                    DeleteAction &deleteAction = deleteActions_[action.order];
                    deleteAction.processOnContext(design);
                    break;
                }
                case ActionType::DRIVE_WITH_CONSTANT:
                {
                    DriveWithConstantAction &driveWithConstantAction = driveWithConstantActions_[action.order];
                    driveWithConstantAction.processOnContext(design);
                    break;
                }
                default:
                    break;
                }
            }
            SNLDesign *newDesign = design;
            for (auto &pathToSet : normalizedAction.paths)
            {
                if (pathToSet.empty())
                {
                    // SNLUniverse::get()->setTopDesign(newDesign);
                    continue;
                }
                else
                {
                    SNLDesign *top = SNLUniverse::get()->getTopDesign();
                    SNLDesign *design = top;
                    std::vector<SNLID::DesignObjectID> path =
                        pathToSet;
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
                        id += "_" + std::to_string(design->getID()) + "_" + std::to_string(inst->getID());
                        pathIds.push_back(inst->getID());
                    }
                    if (pathToSet.size() > 1)
                    {
                        naja::NAJA_OPT::Uniquifier uniquifier(pathIds, id, true);
                        uniquifier.process();
                        design = uniquifier.getPathUniq().back()->getModel();
                    }
                    getInstanceForPath(pathToSet)->setModel(newDesign);
                }
            }
        }
    }
}

SNLInstance *BNE::getInstanceForPath(const std::vector<SNLID::DesignObjectID> &pathToModel)
{
    std::vector<SNLID::DesignObjectID> path = pathToModel;
    SNLDesign *top = SNLUniverse::get()->getTopDesign();
    SNLDesign *designToSet = top;
    SNLInstance *inst = nullptr;
    while (!path.empty())
    {
        SNLID::DesignObjectID name = path.front();
        path.erase(path.begin());
        inst = designToSet->getInstance(name);
        assert(inst);
        designToSet = inst->getModel();
    }
    return inst;
}

void BNE::process()
{
    normalizeActions();
    processNormalizedActions();
}

bool BNE::compareActionsVectorsUnderSameContext(const std::vector<ActionID> &lhs, const std::vector<ActionID> &rhs)
{
    // TODO : add asserstion for same context
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); i++)
    {
        if (lhs[i].type != rhs[i].type)
        {
            return false;
        }
        switch (lhs[i].type)
        {
        case ActionType::DELETE:
        {
            // Static case to delete action for rhs[i] and lhs[i]
            const DeleteAction &lhsDeleteAction = deleteActions_[lhs[i].order];
            const DeleteAction &rhsDeleteAction = deleteActions_[rhs[i].order];
            if (lhsDeleteAction.getToDelete() != rhsDeleteAction.getToDelete())
            {
                return false;
            }
            break;
        }
        case ActionType::DRIVE_WITH_CONSTANT:
        {
            // Static case to drive with constant action for rhs[i] and lhs[i]
            const DriveWithConstantAction &lhsDriveWithConstantAction = driveWithConstantActions_[lhs[i].order];
            const DriveWithConstantAction &rhsDriveWithConstantAction = driveWithConstantActions_[rhs[i].order];
            if (lhsDriveWithConstantAction.getPathToDrive() != rhsDriveWithConstantAction.getPathToDrive() ||
                lhsDriveWithConstantAction.getValue() != rhsDriveWithConstantAction.getValue() ||
                lhsDriveWithConstantAction.getTermToDrive() != rhsDriveWithConstantAction.getTermToDrive() ||
                lhsDriveWithConstantAction.getTopTermToDrive() != rhsDriveWithConstantAction.getTopTermToDrive())
            {
                return false;
            }
            break;
        }
        default:
            return false;
        }
    }
    return true;
}