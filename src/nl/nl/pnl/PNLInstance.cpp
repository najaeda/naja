// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLInstance.h"

#include <sstream>

#include "PNLDesign.h"
#include "PNLInstTerm.h"
#include "PNLBitTerm.h"
#include "NLException.h"
#include "NLName.h"


namespace naja {
namespace NL {

PNLInstance::PNLInstance(PNLDesign* design,
                         PNLDesign* model,
                         const NLName& name)
    : super(), design_(design), model_(model), name_(name) {}

void PNLInstance::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addInstanceAndSetID(this);
  commonPostCreate();
  getModel()->addSlaveInstance(this);
}

PNLInstance* PNLInstance::create(PNLDesign* design,
                                 PNLDesign* model,
                                 const NLName& name) {

  preCreate(design, model, name);
  auto instance = new PNLInstance(design, model, name);
  instance->postCreateAndSetID();
  return instance;
}

void PNLInstance::preCreate(PNLDesign* design,
                            const PNLDesign* model,
                            const NLName& name) {
  super::preCreate();
  if (not design) {
    std::ostringstream reason;
    reason << "malformed PNLInstance ";
    if (name.empty()) {
      reason << " <anonymous>";
    } else {
      // LCOV_EXCL_START
      reason << " with name: " << name.getString();
      // LCOV_EXCL_STOP
    }
    if (model) {
      reason << " and model: " << model->getString();
    } else {
      // LCOV_EXCL_START
      reason << " with NULL model argument";
      // LCOV_EXCL_STOP
    }
    reason << " has a NULL design argument";
    throw NLException(reason.str());
  }
  if (not model) {
    std::ostringstream reason;
    if (name.empty()) {
      // LCOV_EXCL_START
      reason << " <anonymous>";
      // LCOV_EXCL_STOP
    } else {
      // LCOV_EXCL_START
      reason << " with name: " << name.getString();
      // LCOV_EXCL_STOP
    }
    reason << " in design: " << design->getString();
    reason << " has a NULL model argument";
    throw NLException(reason.str());
  }
  if (not name.empty() and design->getInstance(name)) {
    std::string reason =
        "PNLDesign " + design->getString() +
        " contains already a PNLInstance named: " + name.getString();
    throw NLException(reason);
  }
}

void PNLInstance::commonPostCreate() {
  if (not getModel()->isPrimitive()) {
    // Always execute addSlaveInstance after addInstance.
    // addInstance determines the instance ID.
    getModel()->addSlaveInstance(this);
  }
  // create instance terminals
  for (PNLTerm* term : getModel()->getTerms()) {
    PNLBitTerm* bitterm = static_cast<PNLBitTerm*>(term);
    createInstTerm(bitterm);
  }
}

// LCOV_EXCL_START
const char* PNLInstance::getTypeName() const {
  return "PNLInstance";
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
std::string PNLInstance::getString() const {
  return getName().getString();
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
std::string PNLInstance::getDescription() const {
  std::ostringstream description;
  description << "<" << getTypeName();
  description << " " + name_.getString();
  description << " " + std::to_string(getID());
  description << " " + design_->getName().getString();
  description << " " + model_->getName().getString();
  description << ">";
  return description.str();
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void PNLInstance::debugDump(size_t indent,
                            bool recursive,
                            std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
// LCOV_EXCL_STOP

naja::NL::NLID PNLInstance::getNLID() const {
  return PNLDesignObject::getNLID(naja::NL::NLID::Type::Instance, 0, id_, 0);
}

void PNLInstance::preDestroy() {
  if (not getModel()->isPrimitive()) {
    getModel()->removeSlaveInstance(this);
  }
  getDesign()->removeInstance(this);
  commonPreDestroy();
}

void PNLInstance::commonPreDestroy() {
#ifdef PNL_DESTROY_DEBUG
  std::cerr << "commonPreDestroy " << getDescription() << std::endl;
#endif

  // for (const auto& sharedPathsElement : sharedPaths_) {
  //   sharedPathsElement.second->destroyFromInstance();
  // }
  for (auto instTerm : instTerms_) {
    if (instTerm) {
      instTerm->destroyFromInstance();
    }
  }
  // struct destroyInstParameterFromInstance {
  //   void operator()(PNLInstParameter* instParameter) {
  //     instParameter->destroyFromInstance();
  //   }
  // };
  // instParameters_.clear_and_dispose(destroyInstParameterFromInstance());
  super::preDestroy();
}

void PNLInstance::createInstTerm(PNLBitTerm* term) {
  instTerms_.push_back(PNLInstTerm::create(this, term));
}

void PNLInstance::removeInstTerm(PNLBitTerm* term) {
  // removeInstTerm is private so following are internal errors
  assert(term->getDesign() == getModel());
  assert(term->getFlatID() < instTerms_.size());
  auto instTerm = instTerms_[term->getFlatID()];
  if (instTerm) {
    instTerm->destroyFromInstance();
  }
  instTerms_[term->getFlatID()] = nullptr;
}

PNLInstTerm* PNLInstance::getInstTerm(const PNLBitTerm* bitTerm) const {
  if (bitTerm == nullptr) {
    std::string reason = "PNLInstance::getInsTerm error in "
      + getName().getString() + " model: " + getModel()->getName().getString()
      + " bitTerm arg is null";
    throw NLException(reason);
  }
  if (bitTerm->getDesign() != getModel()) {
    std::string reason = "PNLInstance::getInsTerm incoherency: "
      + getName().getString() + " model: " + getModel()->getName().getString()
      + " and " + bitTerm->getString() + " model: " + bitTerm->getDesign()->getName().getString()
      + " should be the same";
    throw NLException(reason);
  }
  assert(bitTerm->getFlatID() < instTerms_.size());
  return instTerms_[bitTerm->getFlatID()];
}

PNLInstTerm* PNLInstance::getInstTerm(const NLID::DesignObjectID termID) const {
  assert(termID < instTerms_.size());
  return instTerms_[termID];
}

bool PNLInstance::isBlackBox() const { return model_->isBlackBox(); }

bool PNLInstance::isPrimitive() const { return model_->isPrimitive(); }

bool PNLInstance::isLeaf() const { return model_->isLeaf(); }

}  // namespace NL
}  // namespace naja