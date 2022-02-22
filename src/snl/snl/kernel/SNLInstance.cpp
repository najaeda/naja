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

#include "SNLInstance.h"

#include <iostream>
#include <sstream>

#include "Card.h"

#include "SNLException.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"

namespace naja { namespace SNL {

SNLInstance::SNLInstance(SNLDesign* design, SNLDesign* model, const SNLName& name):
  super(),
  design_(design),
  model_(model),
  name_(name)
{}

SNLInstance* SNLInstance::create(SNLDesign* design, SNLDesign* model, const SNLName& name) {
  preCreate(design, model, name);
  SNLInstance* instance = new SNLInstance(design, model, name);
  instance->postCreate();
  return instance;
}

SNLSharedPath* SNLInstance::getSharedPath(const SNLSharedPath* tailSharedPath) const {
  auto it = sharedPaths_.find(tailSharedPath);
  if (it != sharedPaths_.end()) {
    return it->second;
  }
  return nullptr;
}

void SNLInstance::addSharedPath(const SNLSharedPath* tailSharedPath) {
}

void SNLInstance::preCreate(SNLDesign* design, const SNLDesign* model, const SNLName& name) {
  super::preCreate();
  if (not design) {
    throw SNLException("malformed SNLInstance creator with NULL design argument");
  }
  if (not model) {
    throw SNLException("malformed SNLInstance creator with NULL model argument");
  }
  if (not name.empty() and design->getInstance(name)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLInstance named: " + name.getString();
    throw SNLException(reason);
  }
}

void SNLInstance::postCreate() {
  super::postCreate();
  getDesign()->addInstance(this);
  if (not getModel()->isPrimitive()) {
    //Always execute addSlaveInstance after addInstance.
    //addInstance determines the instance ID.
    getModel()->addSlaveInstance(this);
  }
  //create instance terminals
  for (SNLTerm* term: getModel()->getTerms()) {
    if (SNLBusTerm* bus = dynamic_cast<SNLBusTerm*>(term)) {
      for (auto bit: bus->getBits()) {
        createInstTerm(bit);
      }
    } else {
      SNLScalarTerm* scalar = static_cast<SNLScalarTerm*>(term);
      createInstTerm(scalar);
    }
  }
}

void SNLInstance::createInstTerm(SNLBitTerm* term) {
  instTerms_.push_back(SNLInstTerm::create(this, term));
}

void SNLInstance::removeInstTerm(SNLBitTerm* term) {
  if (term->getPosition() > instTerms_.size()) {
    throw SNLException("");
  }
  auto instTerm = instTerms_[term->getPosition()];
  if (instTerm) {
    instTerm->destroyFromInstance();
  }
  instTerms_[term->getPosition()] = nullptr;
}

void SNLInstance::commonPreDestroy() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "commonPreDestroy " << getDescription() << std::endl; 
#endif
  for (auto instTerm: instTerms_) {
    if (instTerm) {
      instTerm->destroyFromInstance();
    }
  }
  super::preDestroy();
}

void SNLInstance::destroyFromModel() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying from Model " << getDescription() << std::endl; 
#endif
  getDesign()->removeInstance(this);
  commonPreDestroy();
  delete this;
}

void SNLInstance::destroyFromDesign() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying from Design " << getDescription() << std::endl; 
#endif
  if (not getModel()->isPrimitive()) {
    getModel()->removeSlaveInstance(this);
  }
  commonPreDestroy();
  delete this;
}

void SNLInstance::preDestroy() {
  if (not getModel()->isPrimitive()) {
    getModel()->removeSlaveInstance(this);
  }
  getDesign()->removeInstance(this);
  commonPreDestroy();
}

SNLID SNLInstance::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Instance, 0, id_, 0);
}

SNLInstTerm* SNLInstance::getInstTerm(const SNLBitTerm* term) {
  if (term->getDesign() != getModel()) {
    std::string reason = "SNLInstance::getInsTerm incoherency: "
      + getName().getString() + " model: " + getModel()->getName().getString()
      + " and " + term->getString() + " model: " + term->getDesign()->getName().getString()
      + " should be the same";
    throw SNLException(reason);
  }
  if (term->getPosition() > instTerms_.size()) {
    std::string reason = "SNLInstance::getInsTerm error: size issue";
    throw SNLException(reason);
  }
  return instTerms_[term->getPosition()];
}

SNLCollection<SNLInstTerm*> SNLInstance::getInstTerms() const {
  auto filter = [](const SNLInstTerm* it) {return it != nullptr; };
  return SNLCollection<SNLInstTerm*>(
    new SNLVectorCollection<SNLInstTerm*>(&instTerms_)).getSubCollection(filter);
}

SNLCollection<SNLInstTerm*> SNLInstance::getInstScalarTerms() const {
  auto filter = [](const SNLInstTerm* it) {return it and dynamic_cast<SNLScalarTerm*>(it->getTerm()); };
  return SNLCollection<SNLInstTerm*>(
    new SNLVectorCollection<SNLInstTerm*>(&instTerms_)).getSubCollection(filter);
}

SNLCollection<SNLInstTerm*> SNLInstance::getInstBusTermBits() const {
  auto filter = [](const SNLInstTerm* it) {return it and dynamic_cast<SNLBusTermBit*>(it->getTerm()); };
  return SNLCollection<SNLInstTerm*>(
    new SNLVectorCollection<SNLInstTerm*>(&instTerms_)).getSubCollection(filter);
}

//LCOV_EXCL_START
constexpr const char* SNLInstance::getTypeName() const {
  return "SNLInstance";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstance::getString() const {
  std::ostringstream str; 
  if (not isAnonymous()) {
    str << getName().getString();
  }
  str << "(" << getID() << ")";
  return str.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstance::getDescription() const {
  return "<" + std::string(getTypeName())
    + " " + name_.getString()
    + " " + design_->getName().getString()
    + " " + model_->getName().getString()
    + ">";  
}
//LCOV_EXCL_STOP

Card* SNLInstance::getCard() const {
  Card* card = super::getCard();
  card->addItem(new CardDataItem<const SNLName>("Name", name_));
  card->addItem(new CardDataItem<const SNLDesign*>("Design", design_));
  card->addItem(new CardDataItem<const SNLDesign*>("Model", model_));
  return card;
}

}} // namespace SNL // namespace naja
