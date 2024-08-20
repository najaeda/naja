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

class Action; // Forward declaration of the base class

void BNE::addDeleteAction(const std::vector<SNLID::DesignObjectID> &pathToDelete)
{
    deleteActions_.push_back(DeleteAction(pathToDelete));
    actions_.push_back({ActionType::DELETE, deleteActions_.size() - 1}); // Cast to the base class
    SNLID::DesignObjectID dbID = 0;
    SNLID::DesignObjectID libraryID = 0;
    SNLID::DesignObjectID designID = 0;
    SNLDesign *design = nullptr;
    if (deleteActions_.back().getDeleteContext().empty())
    {
        design = SNLUniverse::get()->getTopDesign();
    }
    else
    {
        design = getInstanceForPath(deleteActions_.back().getDeleteContext())->getModel();
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
        .push_back({actions, deleteActions_.back().getDeleteContext()});
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
                auto path = action.path;
                auto path2 = candidateActionForCompression.path;
                if (path == path2)
                {
                    action.actions.insert(action.actions.end(), candidateActionForCompression.actions.begin(), candidateActionForCompression.actions.end());
                    candidateActionForCompression.actions.clear();
                    candidateActionForCompression.path.clear();
                }
            }
        }
    }
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
                auto path = action.path;
                auto path2 = candidateActionForCompression.path;
                if (path == path2)
                {
                    assert(false);
                }
            }
        }
    }
}
void BNE::normalizeActions()
{
    compressPerPath();
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
                if (compareActionsVectorsUnderSameContext(normalizeActions.actions, candidateActionForCompression.actions)) // create a comparator
                {
                    candidateActionForCompression.actions.clear();
                    normalizeActions.paths.push_back(candidateActionForCompression.path);
                }
            }
            Doid2normalizedAction_[it.first].push_back(normalizeActions);
        }
    }
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
                id += "_" + std::to_string(inst->getID());
                pathIds.push_back(inst->getID());
            }
            if (!normalizedAction.paths.front().empty())
            {
                naja::NAJA_OPT::Uniquifier uniquifier(pathIds, id);
                uniquifier.process();
                design = uniquifier.getPathUniq().back()->getModel();
            }
            for (auto &action : normalizedAction.actions)
            {
                switch (action.type)
                {
                case ActionType::DELETE:
                {
                    const DeleteAction &deleteAction = deleteActions_[action.order];
                    assert(!design->getInstances().empty());
                    design->getInstance(deleteAction.getPathToDelete().back())->destroy();
                    assert(design->getInstance(deleteAction.getPathToDelete().back()) == nullptr);
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
                    SNLUniverse::get()->setTopDesign(newDesign);
                    continue;
                }
                else
                {
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
            if (lhsDeleteAction.getPathToDelete().back() != rhsDeleteAction.getPathToDelete().back())
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