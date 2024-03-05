// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLInstParameter.h"

#include <sstream>

#include "SNLInstance.h"
#include "SNLDesign.h"
#include "SNLParameter.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLInstParameter::SNLInstParameter(SNLInstance* instance, SNLParameter* parameter, const std::string& value):
  instance_(instance), parameter_(parameter), value_(value)
{}

SNLInstParameter* SNLInstParameter::create(SNLInstance* instance, SNLParameter* parameter, const std::string& value) {
  preCreate(instance, parameter);
  SNLInstParameter* instanceParameter = new SNLInstParameter(instance, parameter, value);
  instanceParameter->postCreate();
  return instanceParameter;
}

void SNLInstParameter::postCreate() {
  instance_->addInstParameter(this);
}

void SNLInstParameter::preCreate(SNLInstance* instance, SNLParameter* parameter) {
  if (parameter->getDesign() != instance->getModel()) {
    std::ostringstream reason;
    reason << "In SNLInstance " + instance->getDescription();
    reason << ", cannot add SNLInstParameter for SNLParameter ";
    reason << parameter->getDescription();
    reason << ", contradictory designs: " << parameter->getDesign()->getDescription();
    reason << " and " << instance->getModel()->getDescription();
    throw SNLException(reason.str());
  }
}

void SNLInstParameter::destroy() {
  instance_->removeInstParameter(this);
  delete this;
}

void SNLInstParameter::destroyFromInstance() {
  delete this;
}

SNLName SNLInstParameter::getName() const {
  return parameter_->getName(); 
}

bool SNLInstParameter::deepCompare(const SNLInstParameter* other, std::string& reason) const {
  if (getName() not_eq other->getName()) {
    reason += "In " + getInstance()->getDescription();
    reason += ", different instance parameters: ";
    reason += getDescription() + " and " + other->getDescription();
    return false;
  }
  if (getValue() not_eq other->getValue()) {
    return false;
  }
  return true;
}

//LCOV_EXCL_START
const char* SNLInstParameter::getTypeName() const {
  return "SNLInstParameter";
}
//LCOV_EXCL_STOP
 
//LCOV_EXCL_START
std::string SNLInstParameter::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstParameter::getDescription() const {
  std::ostringstream stream;
  stream << "<" << std::string(getTypeName());
  stream << " " + getName().getString();
  stream << " " << getValue();
  stream << ">";
  return stream.str(); 
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja