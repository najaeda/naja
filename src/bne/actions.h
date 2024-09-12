// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "SNLDesign.h"
#include "SNLDesignTruthTable.h"
#include "SNLID.h"
#include "SNLInstance.h"
#include "SNLLibraryTruthTables.h"
#include "SNLTruthTable.h"

using namespace naja::SNL;

namespace naja::BNE {

// enum type for actions
enum ActionType { DELETE, DRIVE_WITH_CONSTANT, REDUCTION, NONE };

class Action {
 public:
  Action(ActionType type) : type_(type) {}
  virtual void processOnContext(SNLDesign* design) = 0;
  ActionType getType() const { return type_; }
  // get context
  virtual const std::vector<SNLID::DesignObjectID>& getContext() const = 0;
  // comparator
  virtual bool operator==(const Action& action) const = 0;
  // < operator
  virtual bool operator<(const Action& action) const = 0;
  // Virtual destructor
  virtual ~Action() {};

 private:
  ActionType type_;
};

class DriveWithConstantAction : public Action {
 public:
  DriveWithConstantAction(const std::vector<SNLID::DesignObjectID>& context,
                          const SNLID::DesignObjectID& pathToDrive,
                          const SNLID::DesignObjectID& termToDrive,
                          const double& value,
                          SNLBitTerm* topTermToDrive = nullptr)
      : Action(ActionType::DRIVE_WITH_CONSTANT),
        pathToDrive_(pathToDrive),
        termToDrive_(termToDrive),
        value_(value),
        context_(context),
        topTermToDrive_(topTermToDrive) {}
  void changeDriverToLocal0(SNLInstTerm* term);
  void changeDriverToLocal1(SNLInstTerm* term);
  void changeDriverto0Top(SNLBitTerm* term);
  void changeDriverto1Top(SNLBitTerm* term);
  void processOnContext(SNLDesign* design) override;
  const SNLID::DesignObjectID& getPathToDrive() const { return pathToDrive_; }
  const double& getValue() const { return value_; }
  const std::vector<SNLID::DesignObjectID>& getContext() const override {
    return context_;
  }
  const SNLID::DesignObjectID& getTermToDrive() const { return termToDrive_; }
  const SNLBitTerm* getTopTermToDrive() const { return topTermToDrive_; }
  // copy constructor
  DriveWithConstantAction(const DriveWithConstantAction& action)
      : Action(ActionType::DRIVE_WITH_CONSTANT),
        pathToDrive_(action.pathToDrive_),
        termToDrive_(action.termToDrive_),
        value_(action.value_),
        context_(action.context_),
        topTermToDrive_(action.topTermToDrive_) {}
  // comparator
  bool operator==(const Action& action) const override;
  bool operator<(const Action& action) const override;

 private:
  SNLID::DesignObjectID pathToDrive_;
  SNLID::DesignObjectID termToDrive_;
  double value_;
  std::vector<SNLID::DesignObjectID> context_;
  SNLBitTerm* topTermToDrive_ = nullptr;
};

class DeleteAction : public Action {
 public:
  DeleteAction(const std::vector<SNLID::DesignObjectID>& pathToDelete)
      : Action(ActionType::DELETE) {
    toDelete_ = pathToDelete.back();
    context_ = pathToDelete;
    context_.pop_back();
  }
  void processOnContext(SNLDesign* design) override;
  SNLID::DesignObjectID getToDelete() const { return toDelete_; }
  const std::vector<SNLID::DesignObjectID>& getContext() const override {
    return context_;
  }
  // copy constructor
  DeleteAction(const DeleteAction& action)
      : Action(ActionType::DELETE),
        toDelete_(action.toDelete_),
        context_(action.context_) {}
  // comparator
  bool operator==(const Action& action) const override {
    if (action.getType() != ActionType::DELETE) {
      return false;
    }
    const DeleteAction& deleteAction =
        dynamic_cast<const DeleteAction&>(action);
    return toDelete_ == deleteAction.toDelete_;
  }
  bool operator<(const Action& action) const override {
    if (action.getType() != ActionType::DELETE) {
      return getType() < action.getType();
    }
    const DeleteAction& deleteAction =
        dynamic_cast<const DeleteAction&>(action);
    return toDelete_ < deleteAction.toDelete_;
  }

 private:
  SNLID::DesignObjectID toDelete_;
  std::vector<SNLID::DesignObjectID> context_;
};

class ReductionAction : public Action {
 public:
  ReductionAction(
      const std::vector<SNLID::DesignObjectID>& context,
      SNLID::DesignObjectID instance,
      const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result)
      : Action(ActionType::REDUCTION),
        context_(context),
        instance_(instance),
        result_(result) {}
  void replaceInstance(
    SNLInstance* instance,
    const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result);
  void processOnContext(SNLDesign* design) override;
  const std::vector<SNLID::DesignObjectID>& getContext() const override {
    return context_;
  }
  bool operator==(const Action& action) const override {
    if (action.getType() != ActionType::REDUCTION) {
      return false;
    }
    const ReductionAction& reductionAction =
        dynamic_cast<const ReductionAction&>(action);
    return instance_ == reductionAction.instance_ &&
           result_ == reductionAction.result_;
  }
  bool operator<(const Action& action) const override {
    if (action.getType() != ActionType::REDUCTION) {
      return getType() < action.getType();
    }
    const ReductionAction& reductionAction =
        dynamic_cast<const ReductionAction&>(action);
    if (instance_ < reductionAction.instance_) {
      return true;
    } else if (instance_ == reductionAction.instance_) {
      if (result_ < reductionAction.result_) {
        return true;
      }
    }
    return false;
  }
 private:
  const std::vector<SNLID::DesignObjectID> context_;
  const SNLID::DesignObjectID instance_;
  const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes> result_;
};

}  // namespace naja::BNE