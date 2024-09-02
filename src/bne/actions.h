// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLID.h"
#include <vector>
#include "SNLDesign.h"
#include "SNLInstance.h"

using namespace naja::SNL;

// enum type for actions
enum ActionType
{
    DELETE,
    DRIVE_WITH_CONSTANT,
    NONE
};

class Action
{
public:
    Action(ActionType type) : type_(type) {}
    virtual void processOnContext(SNLDesign* design) = 0;
    ActionType getType() { return type_; }
    //get context
    virtual const std::vector<SNLID::DesignObjectID>& getContext() const = 0;
private:
    ActionType type_;
};

class DriveWithConstantAction : public Action
{       
public:
    DriveWithConstantAction(const std::vector<SNLID::DesignObjectID> &context, 
        const SNLID::DesignObjectID &pathToDrive, const SNLID::DesignObjectID &termToDrive, 
        const double &value, SNLBitTerm* topTermToDrive = nullptr) : Action(ActionType::DRIVE_WITH_CONSTANT), pathToDrive_(pathToDrive), 
        termToDrive_(termToDrive), value_(value), context_(context), topTermToDrive_(topTermToDrive) {}
    void processOnContext(SNLDesign* design) override;
    const SNLID::DesignObjectID& getPathToDrive() const { return pathToDrive_; }
    const double& getValue() const { return value_; }
    const std::vector<SNLID::DesignObjectID>& getContext() const override { return context_; }
    const SNLID::DesignObjectID& getTermToDrive() const { return termToDrive_; }
    const SNLBitTerm* getTopTermToDrive() const { return topTermToDrive_; }
    //copy constructor
    DriveWithConstantAction(const DriveWithConstantAction &action) : Action(ActionType::DRIVE_WITH_CONSTANT),
         pathToDrive_(action.pathToDrive_), termToDrive_(action.termToDrive_), value_(action.value_), 
            context_(action.context_), topTermToDrive_(action.topTermToDrive_) {}   
    //comparator
    bool operator==(const DriveWithConstantAction &action) const { return pathToDrive_ == action.pathToDrive_ && 
        termToDrive_ == action.termToDrive_; }
private:
    SNLID::DesignObjectID pathToDrive_;
    SNLID::DesignObjectID termToDrive_;
    double value_;
    std::vector<SNLID::DesignObjectID> context_;
    SNLBitTerm* topTermToDrive_ = nullptr;
};

class DeleteAction : public Action
{
public:
    DeleteAction(const std::vector<SNLID::DesignObjectID> &pathToDelete) : Action(ActionType::DELETE) { toDelete_ = pathToDelete.back(); context_ = pathToDelete; context_.pop_back(); }
    void processOnContext(SNLDesign* design) override;
    SNLID::DesignObjectID getToDelete() const { return toDelete_; }
    const std::vector<SNLID::DesignObjectID>& getContext() const override { return context_; }
    //copy constructor
    DeleteAction(const DeleteAction &action) : Action(ActionType::DELETE), toDelete_(action.toDelete_), context_(action.context_) {}
    //comparator
    bool operator==(const DeleteAction &action) const { return toDelete_ == action.toDelete_; }
private:
    SNLID::DesignObjectID toDelete_;
    std::vector<SNLID::DesignObjectID> context_;
};

