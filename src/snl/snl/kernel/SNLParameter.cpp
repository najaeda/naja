// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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