/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SNLParameter.h"

#include <sstream>

#include "SNLDesign.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLParameter::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string SNLParameter::Type::getString() const {
  switch (typeEnum_) {
    case Type::Decimal: return "Decimal";
    case Type::Binary: return "Binary";
    case Type::Boolean: return "Boolean";
    case Type::String: return "String";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

SNLParameter::SNLParameter(SNLDesign* design, const SNLName& name, Type type, const std::string& value):
  design_(design), name_(name), type_(type), value_(value)
{}

SNLParameter* SNLParameter::create(SNLDesign* design, const SNLName& name, Type type, const std::string& value) {
  preCreate(design, name);
  SNLParameter* parameter = new SNLParameter(design, name, type, value);
  parameter->postCreate();
  return parameter;
}

void SNLParameter::postCreate() {
  design_->addParameter(this);
}

void SNLParameter::preCreate(SNLDesign* design, const SNLName& name) {
  if (design->getParameter(name)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLParameter named: " + name.getString();
    throw SNLException(reason);
  }
}

void SNLParameter::destroy() {
  design_->removeParameter(this);
  delete this;
}

void SNLParameter::destroyFromDesign() {
  delete this;
}

bool SNLParameter::deepCompare(const SNLParameter* other, std::string& reason) const {
  if (getName() != other->getName()) {
    return false;
  }
  if (getValue() not_eq other->getValue()) {
    return false;
  }
  return true;
}

//LCOV_EXCL_START
const char* SNLParameter::getTypeName() const {
  return "SNLParameter";
}
//LCOV_EXCL_STOP
 
//LCOV_EXCL_START
std::string SNLParameter::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLParameter::getDescription() const {
  std::ostringstream stream;
  stream << "<" << std::string(getTypeName());
  stream << " " + getType().getString();
  stream << " " + getName().getString();
  stream << " " << getValue();
  stream << ">";
  return stream.str(); 
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja