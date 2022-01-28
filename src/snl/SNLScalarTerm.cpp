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

#include "SNLScalarTerm.h"

#include <sstream>

#include "SNLException.h"
#include "SNLDesign.h"

namespace SNL {

SNLScalarTerm::SNLScalarTerm(SNLDesign* design, const Direction& direction, const SNLName& name):
  super(),
  design_(design),
  name_(name),
  direction_(direction)
{}

SNLScalarTerm* SNLScalarTerm::create(SNLDesign* design, const Direction& direction, const SNLName& name) {
  preCreate(design, name);
  SNLScalarTerm* net = new SNLScalarTerm(design, direction, name);
  net->postCreate();
  return net;
}

void SNLScalarTerm::preCreate(SNLDesign* design, const SNLName& name) {
  super::preCreate();
  if (not design) {
    throw SNLException("malformed SNLScalarTerm creator with NULL design argument");
  }
  if (not name.empty() and design->getTerm(name)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLScalarTerm named: " + name.getString();
    throw SNLException(reason);
  }
}

void SNLScalarTerm::postCreate() {
  super::postCreate();
  getDesign()->addTerm(this);
}

void SNLScalarTerm::commonPreDestroy() {
  super::preDestroy();
}

void SNLScalarTerm::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLScalarTerm::preDestroy() {
  commonPreDestroy();
  getDesign()->removeTerm(this);
}

SNLID SNLScalarTerm::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Term, id_, 0, 0);
}

constexpr const char* SNLScalarTerm::getTypeName() const {
  return "SNLScalarTerm";
}
 
std::string SNLScalarTerm::getString() const {
  std::ostringstream str;
  if (not isAnonymous()) {
    str << getName().getString();
  }
  str << "(" << getID() << ")";
  return str.str();
}

std::string SNLScalarTerm::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_.getString() + " " + design_->getName().getString() + ">";  
}

}
