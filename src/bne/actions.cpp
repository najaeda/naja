// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "actions.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <ranges>
#include <set>
#include <stack>
#include <vector>
#include "SNLDesignModeling.h"
#include "SNLDesignTruthTable.h"
#include "SNLLibraryTruthTables.h"
#include "SNLScalarNet.h"
#include "SNLTruthTable.h"
#include "Utils.h"

using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::BNE;

void DriveWithConstantAction::changeDriverToLocal0(SNLInstTerm* term) {
  term->setNet(nullptr);
  std::string name(std::string("logic0_naja"));
  auto netName = SNLName(name + "_net");
  SNLNet* assign0 = term->getDesign()->getNet(netName);
  if (nullptr == assign0) {
    assign0 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign0->setType(naja::SNL::SNLNet::Type::Supply0);
  term->setNet(assign0);
  SNLTruthTable tt(0, 0);
  // find primitives library
  if (term->getDB()->getPrimitiveLibraries().size() != 1) {
    // LCOV_EXCL_START
    throw SNLException("There should be only one primitive library");
    // LCOV_EXCL_STOP
  }
  auto primitives = *term->getDB()->getPrimitiveLibraries().begin();
  auto logic0 =
      SNLLibraryTruthTables::getDesignForTruthTable(primitives, tt).first;

  SNLInstance* logic0Inst = term->getDesign()->getInstance(SNLName(name));
  if (nullptr == logic0Inst) {
    if (logic0 == nullptr) {
      // LCOV_EXCL_START
      throw SNLException("No logic0 design found");
      // LCOV_EXCL_STOP
    }
    logic0Inst = SNLInstance::create(term->getDesign(), logic0, SNLName(name));
  }
  (*logic0Inst->getInstTerms().begin())->setNet(assign0);
}

void DriveWithConstantAction::changeDriverToLocal1(SNLInstTerm* term) {
  term->setNet(nullptr);
  std::string name(std::string("logic1_naja"));
  auto netName = SNLName(name + "_net");
  SNLNet* assign1 = term->getDesign()->getNet(netName);
  if (nullptr == assign1) {
    assign1 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign1->setType(naja::SNL::SNLNet::Type::Supply1);
  term->setNet(assign1);
  SNLTruthTable tt(0, 1);

  // find primitives library
  if (term->getDB()->getPrimitiveLibraries().size() != 1) {
    // LCOV_EXCL_START
    throw SNLException("There should be only one primitive library");
    // LCOV_EXCL_STOP
  }
  auto primitives = *term->getDB()->getPrimitiveLibraries().begin();
  auto logic1 =
      SNLLibraryTruthTables::getDesignForTruthTable(primitives, tt).first;
  SNLInstance* logic1Inst = term->getDesign()->getInstance(SNLName(name));
  if (nullptr == logic1Inst) {
    if (logic1 == nullptr) {
      // LCOV_EXCL_START
      throw SNLException("No logic1 design found");
      // LCOV_EXCL_STOP
    }
    logic1Inst = SNLInstance::create(term->getDesign(), logic1, SNLName(name));
  }
  (*logic1Inst->getInstTerms().begin())->setNet(assign1);
}

void DriveWithConstantAction::changeDriverto0Top(SNLBitTerm* term) {
  term->setNet(nullptr);
  std::string name(std::string("logic0_naja"));
  auto netName = SNLName(name + "_net");
  SNLNet* assign0 = term->getDesign()->getNet(netName);
  if (nullptr == assign0) {
    assign0 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign0->setType(naja::SNL::SNLNet::Type::Supply0);
  term->setNet(assign0);
  SNLTruthTable tt(0, 0);
  auto logic0 = SNLLibraryTruthTables::getDesignForTruthTable(
                    *(term->getDB()->getPrimitiveLibraries().begin()), tt)
                    .first;
  SNLInstance* logic0Inst = term->getDesign()->getInstance(SNLName(name));
  if (nullptr == logic0Inst) {
    logic0Inst = SNLInstance::create(term->getDesign(), logic0, SNLName(name));
  }
  (*logic0Inst->getInstTerms().begin())->setNet(assign0);
}

void DriveWithConstantAction::changeDriverto1Top(SNLBitTerm* term) {
  term->setNet(nullptr);
  std::string name(std::string("logic1_naja"));
  auto netName = SNLName(name + "_net");
  SNLNet* assign1 = term->getDesign()->getNet(netName);
  if (nullptr == assign1) {
    assign1 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign1->setType(naja::SNL::SNLNet::Type::Supply1);
  term->setNet(assign1);
  SNLTruthTable tt(0, 1);
  auto logic1 = SNLLibraryTruthTables::getDesignForTruthTable(
                    *(term->getDB()->getPrimitiveLibraries().begin()), tt)
                    .first;
  SNLInstance* logic1Inst = term->getDesign()->getInstance(SNLName(name));
  if (nullptr == logic1Inst) {
    logic1Inst = SNLInstance::create(term->getDesign(), logic1, SNLName(name));
  }
  (*logic1Inst->getInstTerms().begin())->setNet(assign1);
}

void DriveWithConstantAction::processOnContext(SNLDesign* design) {
  if (value_ == 0) {
    if (context_.empty() && pathToDrive_ == (unsigned)(-1) &&
        termToDrive_ == (unsigned)(-1)) {
      assert(topTermToDrive_ != nullptr);
      changeDriverto0Top(topTermToDrive_);
    } else {
      changeDriverToLocal0(
          design->getInstance(pathToDrive_)->getInstTerm(termToDrive_));
    }
  } else if (value_ == 1) {
    if (context_.empty() && pathToDrive_ == (unsigned)(-1) &&
        termToDrive_ == (unsigned)(-1)) {
      assert(topTermToDrive_ != nullptr);
      changeDriverto1Top(topTermToDrive_);
    } else {
      changeDriverToLocal1(
          design->getInstance(pathToDrive_)->getInstTerm(termToDrive_));
    }
  } else {
    // LCOV_EXCL_START
    throw SNLException("Value should be 0 or 1");
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
         termToDrive_ == driveWithConstantAction.termToDrive_;
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
    if (termToDrive_ < driveWithConstantAction.termToDrive_) {
      return true;
    } else if (termToDrive_ == driveWithConstantAction.termToDrive_) {
      if (value_ < driveWithConstantAction.value_) {
        return true;
      }
    }
  }
  return false;
  // LCOV_EXCL_STOP
}

void DeleteAction::processOnContext(SNLDesign* design) {
  design->getInstance(toDelete_)->destroy();
}

bool DeleteAction::operator==(const Action& action) const {
  if (action.getType() != ActionType::DELETE) {
    // LCOV_EXCL_START
    return false;
    // LCOV_EXCL_STOP
  }
  const DeleteAction& deleteAction = dynamic_cast<const DeleteAction&>(action);
  return toDelete_ == deleteAction.toDelete_;
}

bool DeleteAction::operator<(const Action& action) const {
  if (action.getType() != ActionType::DELETE) {
    // LCOV_EXCL_START
    return getType() < action.getType();
    // LCOV_EXCL_STOP
  }
  const DeleteAction& deleteAction = dynamic_cast<const DeleteAction&>(action);
  return toDelete_ < deleteAction.toDelete_;
}

void ReductionAction::replaceInstance(
    SNLInstance* instance,
    const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result) {
  SNLDesign* design = instance->getDesign();
  SNLDesign* reducedDesign = result.first;
  SNLInstance* reducedInstance = SNLInstance::create(
      design, reducedDesign,
      SNLName(std::string(instance->getName().getString()) + "_reduced"));
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
    if (bitNet->isConstant() || reducedInstTerms.empty()) {
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