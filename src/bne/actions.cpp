// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "actions.h"

#include <iostream>
#include <limits>
#include <vector>

#include "NLLibraryTruthTables.h"
#include "SNLScalarNet.h"
#include "SNLTruthTable.h"
#include "SNLDesignModeling.h"
#include "NLDB0.h"
#include "Utils.h"

using namespace naja::DNL;
using namespace naja::NL;
using namespace naja::BNE;

namespace {

constexpr NLID::DesignObjectID InvalidPathToDrive =
    std::numeric_limits<NLID::DesignObjectID>::max();
constexpr size_t InvalidFlatTermToDrive = std::numeric_limits<size_t>::max();

SNLBitNet* getOrCreateConstantNet(
    SNLDesign* design,
    const NLName& netName,
    const NLName& instanceName,
    NLLogicValue value) {
  auto* net = dynamic_cast<SNLBitNet*>(design->getNet(netName));
  if (!net) {
    net = SNLScalarNet::create(design, netName);
  }
  auto* instance = design->getInstance(instanceName);
  if (!instance) {
    instance = SNLDesignModeling::createConstantDriver(
      design, NLLogicVector::filled(1, value),
      NLConstantDriverKind::Supply, instanceName);
    instance->setTermNet(NLDB0::getConstOutput(instance->getModel()), net);
  }
  return net;
}

}

void DriveWithConstantAction::changeDriverToLocal0(SNLInstTerm* term) {
  term->setNet(nullptr);
  std::string name(std::string("logic0_naja"));
  auto netName = NLName(name + "_net");
  auto* assign0 = getOrCreateConstantNet(
    term->getDesign(), netName, NLName(name), NLLogicValue::Zero);
  term->setNet(assign0);
}

void DriveWithConstantAction::changeDriverToLocal1(SNLInstTerm* term) {
  term->setNet(nullptr);
  std::string name(std::string("logic1_naja"));
  auto netName = NLName(name + "_net");
  auto* assign1 = getOrCreateConstantNet(
    term->getDesign(), netName, NLName(name), NLLogicValue::One);
  term->setNet(assign1);
}

void DriveWithConstantAction::changeDriverto0Top(SNLBitTerm* term) {
  term->setNet(nullptr);
  std::string name(std::string("logic0_naja"));
  auto netName = NLName(name + "_net");
  auto* assign0 = getOrCreateConstantNet(
    term->getDesign(), netName, NLName(name), NLLogicValue::Zero);
  term->setNet(assign0);
}

void DriveWithConstantAction::changeDriverto1Top(SNLBitTerm* term) {
  term->setNet(nullptr);
  std::string name(std::string("logic1_naja"));
  auto netName = NLName(name + "_net");
  auto* assign1 = getOrCreateConstantNet(
    term->getDesign(), netName, NLName(name), NLLogicValue::One);
  term->setNet(assign1);
}

void DriveWithConstantAction::processOnContext(SNLDesign* design) {
  if (value_ == 0) {
    if (context_.empty() && pathToDrive_ == InvalidPathToDrive &&
        flatTermToDrive_ == InvalidFlatTermToDrive) {
      assert(topTermToDrive_ != nullptr);
      changeDriverto0Top(topTermToDrive_);
    } else {
      changeDriverToLocal0(
          design->getInstance(pathToDrive_)->getInstTermByFlatID(flatTermToDrive_));
    }
  } else if (value_ == 1) {
    if (context_.empty() && pathToDrive_ == InvalidPathToDrive &&
        flatTermToDrive_ == InvalidFlatTermToDrive) {
      assert(topTermToDrive_ != nullptr);
      changeDriverto1Top(topTermToDrive_);
    } else {
      changeDriverToLocal1(
          design->getInstance(pathToDrive_)->getInstTermByFlatID(flatTermToDrive_));
    }
  } else {
    // LCOV_EXCL_START
    throw NLException("Value should be 0 or 1");
    // LCOV_EXCL_STOP
  }
}

bool DriveWithConstantAction::operator==(const Action& action) const {
  // LCOV_EXCL_START
  if (action.getType() != ActionType::DRIVE_WITH_CONSTANT) {
    return false;
  }
  const DriveWithConstantAction& driveWithConstantAction =
      dynamic_cast<const DriveWithConstantAction&>(action);
  return pathToDrive_ == driveWithConstantAction.pathToDrive_ &&
         flatTermToDrive_ == driveWithConstantAction.flatTermToDrive_;
  // LCOV_EXCL_STOP
}
bool DriveWithConstantAction::operator<(const Action& action) const {
  // LCOV_EXCL_START
  if (action.getType() != ActionType::DRIVE_WITH_CONSTANT) {
    return getType() < action.getType();
  }
  const DriveWithConstantAction& driveWithConstantAction =
      dynamic_cast<const DriveWithConstantAction&>(action);
  if (topTermToDrive_ != nullptr) {
    if (topTermToDrive_ < driveWithConstantAction.topTermToDrive_) {
      return true;
    } else if (topTermToDrive_ == driveWithConstantAction.topTermToDrive_) {
      if (value_ < driveWithConstantAction.value_) {
        return true;
      }
    }
    return false;
  }
  if (pathToDrive_ < driveWithConstantAction.pathToDrive_) {
    return true;
  } else if (pathToDrive_ == driveWithConstantAction.pathToDrive_) {
    if (flatTermToDrive_ < driveWithConstantAction.flatTermToDrive_) {
      return true;
    } else if (flatTermToDrive_ == driveWithConstantAction.flatTermToDrive_) {
      if (value_ < driveWithConstantAction.value_) {
        return true;
      }
    }
  }
  return false;
  // LCOV_EXCL_STOP
}

DeleteAction::DeleteAction(
    const std::vector<NLID::DesignObjectID>& pathToDelete)
    : Action(ActionType::DELETE_ACTION) {
  assert(!pathToDelete.empty());
  toDelete_ = pathToDelete.back();
  context_ = pathToDelete;
  context_.pop_back();
}

void DeleteAction::processOnContext(SNLDesign* design) {
  design->getInstance(toDelete_)->destroy();
}

bool DeleteAction::operator==(const Action& action) const {
  if (action.getType() != ActionType::DELETE_ACTION) {
    // LCOV_EXCL_START
    return false;
    // LCOV_EXCL_STOP
  }
  const DeleteAction& deleteAction = dynamic_cast<const DeleteAction&>(action);
  return toDelete_ == deleteAction.toDelete_;
}

bool DeleteAction::operator<(const Action& action) const {
  if (action.getType() != ActionType::DELETE_ACTION) {
    // LCOV_EXCL_START
    return getType() < action.getType();
    // LCOV_EXCL_STOP
  }
  const DeleteAction& deleteAction = dynamic_cast<const DeleteAction&>(action);
  return toDelete_ < deleteAction.toDelete_;
}

void ReductionAction::replaceInstance(
    SNLInstance* instance,
    const std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>& result) {
  SNLDesign* design = instance->getDesign();
  SNLDesign* reducedDesign = result.first;
  SNLInstance* reducedInstance = SNLInstance::create(
      design, reducedDesign,
      NLName(std::string(instance->getName().getString()) + "_reduced"));
  std::vector<SNLInstTerm*> reducedInstTerms;
  SNLInstTerm* output = nullptr;
  SNLInstTerm* reducedOutput = nullptr;
  for (auto term : reducedInstance->getInstTerms()) {
    if (term->getDirection() != SNLInstTerm::Direction::Input) {
      reducedOutput = term;
      continue;
    }
    reducedInstTerms.push_back(term);
  }
  size_t index = 0;
  size_t originNonConstantIndex = 0;
  for (auto term : instance->getInstTerms()) {
    if (term->getDirection() != SNLInstTerm::Direction::Input) {
      output = term;
      break;
    }
  }
  for (auto term : instance->getInstTerms()) {
    SNLBitNet* bitNet = term->getNet();
    term->setNet(nullptr);
    if (SNLDesignModeling::getConstantValue(bitNet) || reducedInstTerms.empty()) {
      continue;
    }
    originNonConstantIndex++;
    if (std::find(result.second.begin(), result.second.end(),
                  originNonConstantIndex) != result.second.end()) {
      continue;
    }
    reducedInstTerms[index]->setNet(bitNet);
    index++;
    if (index == reducedInstTerms.size()) {
      break;
    }
  }
  reducedOutput->setNet(output->getNet());
  output->setNet(nullptr);
  instance->destroy();
}

void ReductionAction::processOnContext(SNLDesign* design) {
  auto instance = design->getInstance(instance_);
  replaceInstance(instance, result_);
}

bool ReductionAction::operator==(const Action& action) const {
  // LCOV_EXCL_START
  if (action.getType() != ActionType::REDUCTION) {
    return false;
  }
  const ReductionAction& reductionAction =
      dynamic_cast<const ReductionAction&>(action);
  return instance_ == reductionAction.instance_ &&
         result_ == reductionAction.result_;
  // LCOV_EXCL_STOP
}

bool ReductionAction::operator<(const Action& action) const {
  // LCOV_EXCL_START
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
  // LCOV_EXCL_STOP
}
