// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLDesignModeling.h"
#include "NLLibraryTruthTables.h"
#include "SNLTruthTable.h"

using namespace naja::NL;

namespace naja::BNE {

// enum type for actions
enum ActionType { DELETE, DRIVE_WITH_CONSTANT, REDUCTION, NONE };

class Action {
 public:
  /**
   * \brief Create an Action.
   * \param type The type of the action.
   * \return The created Action.
   */
  Action(ActionType type) : type_(type) {}
  /**
   * \brief Process the action on the context.
   * \param design The design to process the action on.
   */
  virtual void processOnContext(SNLDesign* design) = 0;
  /**
   * \brief Get the type of the action.
   * \return The type of the action.
   */
  ActionType getType() const { return type_; }
  // get context
  /**
   * \brief Get the context of the action.
   * \return The context of the action.
   */
  virtual const std::vector<NLID::DesignObjectID>& getContext() const = 0;
  // comparator
  virtual bool operator==(const Action& action) const = 0;
  // < operator
  virtual bool operator<(const Action& action) const = 0;
  // Virtual destructor
  virtual ~Action() {};

  virtual void destroy() = 0;

 private:
  ActionType type_;
};

class DriveWithConstantAction : public Action {
 public:
  DriveWithConstantAction(const std::vector<NLID::DesignObjectID>& context,
                          const NLID::DesignObjectID& pathToDrive,
                          const NLID::DesignObjectID& termToDrive,
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
  const NLID::DesignObjectID& getPathToDrive() const { return pathToDrive_; }
  const double& getValue() const { return value_; }
  const std::vector<NLID::DesignObjectID>& getContext() const override {
    return context_;
  }
  const NLID::DesignObjectID& getTermToDrive() const { return termToDrive_; }
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
  void destroy() override { delete this; }
 private:
  NLID::DesignObjectID pathToDrive_;
  NLID::DesignObjectID termToDrive_;
  double value_;
  std::vector<NLID::DesignObjectID> context_;
  SNLBitTerm* topTermToDrive_ = nullptr;
};

class DeleteAction : public Action {
 public:
  DeleteAction(const std::vector<NLID::DesignObjectID>& pathToDelete);
  void processOnContext(SNLDesign* design) override;
  NLID::DesignObjectID getToDelete() const { return toDelete_; }
  const std::vector<NLID::DesignObjectID>& getContext() const override {
    return context_;
  }
  // copy constructor
  DeleteAction(const DeleteAction& action)
      : Action(ActionType::DELETE),
        toDelete_(action.toDelete_),
        context_(action.context_) {}
  // comparator
  bool operator==(const Action& action) const override;
  bool operator<(const Action& action) const override;
  void destroy() override { delete this; }
 private:
  NLID::DesignObjectID toDelete_;
  std::vector<NLID::DesignObjectID> context_;
};

class ReductionAction : public Action {
 public:
  ReductionAction(
      const std::vector<NLID::DesignObjectID>& context,
      NLID::DesignObjectID instance,
      const std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>& result)
      : Action(ActionType::REDUCTION),
        context_(context),
        instance_(instance),
        result_(result) {}
  void replaceInstance(
    SNLInstance* instance,
    const std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>& result);
  void processOnContext(SNLDesign* design) override;
  const std::vector<NLID::DesignObjectID>& getContext() const override {
    return context_;
  }
  bool operator==(const Action& action) const override;
  bool operator<(const Action& action) const override;
  void destroy() override { delete this; }
 private:
  const std::vector<NLID::DesignObjectID> context_;
  const NLID::DesignObjectID instance_;
  const std::pair<SNLDesign*, NLLibraryTruthTables::Indexes> result_;
};

}  // namespace naja::BNE
