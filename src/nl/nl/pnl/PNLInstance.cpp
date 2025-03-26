// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLInstance.h"

#include <sstream>

#include "PNLDesign.h"

#include "NLException.h"

#include "NLName.h"

namespace naja { namespace NL {

PNLInstance::PNLInstance(PNLDesign* design, PNLDesign* model, const NLName& name):
  super(),
  design_(design),
  model_(model),
  name_(name)
{}

void PNLInstance::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addInstanceAndSetID(this);
  commonPostCreate();
}

PNLInstance* PNLInstance::create(PNLDesign* design, PNLDesign* model, const NLName& name) {
  preCreate(design, model, name);
  auto instance = new PNLInstance(design, model, name);
  instance->postCreateAndSetID();
  return instance;
}

void PNLInstance::preCreate(PNLDesign* design, const PNLDesign* model, const NLName& name) {
  super::preCreate();
  if (not design) {
    std::ostringstream reason;
    reason << "malformed SNLInstance ";
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
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLInstance named: " + name.getString();
    throw NLException(reason);
  }
}

void PNLInstance::commonPostCreate() {
  // if (not getModel()->isPrimitive()) {
  //   //Always execute addSlaveInstance after addInstance.
  //   //addInstance determines the instance ID.
  //   getModel()->addSlaveInstance(this);
  // }
  // //create instance terminals
  // for (SNLTerm* term: getModel()->getTerms()) {
  //   if (SNLBusTerm* bus = dynamic_cast<SNLBusTerm*>(term)) {
  //     for (auto bit: bus->getBits()) {
  //       createInstTerm(bit);
  //     }
  //   } else {
  //     SNLScalarTerm* scalar = static_cast<SNLScalarTerm*>(term);
  //     createInstTerm(scalar);
  //   }
  // }
}

//LCOV_EXCL_START
const char* PNLInstance::getTypeName() const {
  return "PNLInstance";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLInstance::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
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
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void PNLInstance::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

naja::NL::NLID PNLInstance::getNLID() const {
  return PNLDesignObject::getNLID(naja::NL::NLID::Type::Instance, 0, id_, 0);
}

}} // namespace NL // namespace naja