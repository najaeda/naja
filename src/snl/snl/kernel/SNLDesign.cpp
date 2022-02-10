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

#include "SNLDesign.h"

#include <iostream>
#include <sstream>

#include "Card.h"

#include "SNLException.h"
#include "SNLDB.h" 
#include "SNLLibrary.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

namespace naja { namespace SNL {

SNLDesign::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string SNLDesign::Type::getString() const {
  switch (typeEnum_) {
    case Type::Standard: return "Standard";
    case Type::Blackbox: return "Blackbox";
    case Type::Primitive: return "Primitive";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

SNLDesign* SNLDesign::create(SNLLibrary* library, const SNLName& name) {
  preCreate(library, Type::Standard, name);
  SNLDesign* design = new SNLDesign(library, Type::Standard, name);
  design->postCreate();
  return design;
}

SNLDesign* SNLDesign::create(SNLLibrary* library, const Type& type, const SNLName& name) {
  preCreate(library, type, name);
  SNLDesign* design = new SNLDesign(library, type, name);
  design->postCreate();
  return design;
}

SNLDesign::SNLDesign(SNLLibrary* library, const Type& type, const SNLName& name):
  super(),
  name_(name),
  type_(type),
  library_(library)
{}

void SNLDesign::preCreate(const SNLLibrary* library, const Type& type, const SNLName& name) {
  super::preCreate();
  if (type == Type::Primitive and not library->isPrimitives()) {
    throw SNLException("non compatible types in design constructor");
  }
  //test if design with same name exists in library
  if (not name.empty()) {
  }
}

void SNLDesign::postCreate() {
  super::postCreate();
  library_->addDesign(this);
}

void SNLDesign::commonPreDestroy() {
#if DEBUG
  std::cerr << "commonPreDestroy: " << getString() << std::endl;
#endif
  struct destroyTermFromDesign {
    void operator()(SNLTerm* term) {
      term->destroyFromDesign();
    }
  };
  terms_.clear_and_dispose(destroyTermFromDesign());

  struct destroyInstanceFromDesign {
    void operator()(SNLInstance* instance) {
      instance->destroyFromDesign();
    }
  };
  instances_.clear_and_dispose(destroyInstanceFromDesign());

  if (not isPrimitive()) {
    struct destroySlaveInstanceFromModel {
      void operator()(SNLInstance* instance) {
        instance->destroyFromModel();
      }
    };
    slaveInstances_.clear_and_dispose(destroySlaveInstanceFromModel());
  }

  struct destroyNetFromDesign {
    void operator()(SNLNet* net) {
      net->destroyFromDesign();
    }
  };
  nets_.clear_and_dispose(destroyNetFromDesign());

  struct destroyParameterFromDesign {
    void operator()(SNLParameter* parameter) {
      parameter->destroyFromDesign();
    }
  };
  parameters_.clear_and_dispose(destroyParameterFromDesign());

  super::preDestroy();
}

void SNLDesign::destroyFromLibrary() {
  commonPreDestroy();
  delete this;
}

void SNLDesign::preDestroy() {
  if (isPrimitive()) {
    //FIXME: Error
  }
  library_->removeDesign(this);
  commonPreDestroy();
}

void SNLDesign::addTerm(SNLTerm* term) {
  assert(dynamic_cast<SNLScalarTerm*>(term) or dynamic_cast<SNLBusTerm*>(term));

  if (terms_.empty()) {
    term->setID(0);
  } else {
    auto it = terms_.rbegin();
    SNLTerm* lastTerm = &(*it);
    SNLID::DesignObjectID termID = lastTerm->getID()+1;
    size_t position = 0;
    if (SNLScalarTerm* scalarTerm = dynamic_cast<SNLScalarTerm*>(lastTerm)) {
      position = scalarTerm->getPosition() + 1;
    } else {
      SNLBusTerm* busTerm = static_cast<SNLBusTerm*>(lastTerm);
      position = busTerm->position_ + busTerm->getSize();
    }
    term->setID(termID);
    term->setPosition(position);
  }
  terms_.insert(*term);
  if (not term->getName().empty()) {
    termNameIDMap_[term->getName()] = term->getID();
  }

  //Create corresponding instance terminals in slave instances
  for (auto instance: getSlaveInstances()) {
    if (SNLScalarTerm* scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
      instance->createInstTerm(scalarTerm);
    } else {
      SNLBusTerm* busTerm = static_cast<SNLBusTerm*>(term);
      for (auto bit: busTerm->getBits()) {
        instance->createInstTerm(bit);
      }
    }
  }
}

void SNLDesign::removeTerm(SNLTerm* term) {
  //Remove corresponding instance terminals in slave instances
  for (auto instance: getSlaveInstances()) {
    if (SNLBusTerm* bus = dynamic_cast<SNLBusTerm*>(term)) {
      for (auto bit: bus->getBits()) {
        instance->removeInstTerm(bit);
      }
    } else {
      SNLBitTerm* bitTerm = static_cast<SNLBitTerm*>(term);
      instance->removeInstTerm(bitTerm);
    }
  }
  if (dynamic_cast<SNLBusTerm*>(term) or dynamic_cast<SNLScalarTerm*>(term)) {
    if (not term->getName().empty()) {
      termNameIDMap_.erase(term->getName());
    }
    terms_.erase(*term);
  }
}

SNLTerm* SNLDesign::getTerm(SNLID::DesignObjectID id) const {
  auto it = terms_.find(
      SNLID(SNLID::Type::Term, getDB()->getID(), getLibrary()->getID(), getID(), id, 0, 0),
      SNLIDComp<SNLTerm>());
  if (it != terms_.end()) {
    return const_cast<SNLTerm*>(&*it);
  }
  return nullptr;
}

SNLTerm* SNLDesign::getTerm(const SNLName& name) const {
  auto it = termNameIDMap_.find(name);
  if (it != termNameIDMap_.end()) {
    SNLID::DesignObjectID id = it->second;
    return getTerm(id);
  }
  return nullptr;
}

SNLScalarTerm* SNLDesign::getScalarTerm(const SNLName& name) const {
  return dynamic_cast<SNLScalarTerm*>(getTerm(name));
}

SNLBusTerm* SNLDesign::getBusTerm(const SNLName& name) const {
  return dynamic_cast<SNLBusTerm*>(getTerm(name));
}

SNLCollection<SNLTerm*> SNLDesign::getTerms() const {
  return SNLCollection<SNLTerm*>(new SNLIntrusiveSetCollection<SNLTerm, SNLDesignTermsHook>(&terms_));
}

SNLCollection<SNLBusTerm*> SNLDesign::getBusTerms() const {
  return getTerms().getSubCollection<SNLBusTerm*>();
}

SNLCollection<SNLScalarTerm*> SNLDesign::getScalarTerms() const {
  return getTerms().getSubCollection<SNLScalarTerm*>();
}

SNLCollection<SNLBitTerm*> SNLDesign::getBitTerms() const {
  auto flattener = [](const SNLBusTerm* b) { return b->getBits(); };
  return getTerms().getFlatCollection<SNLBusTerm*, SNLBusTermBit*, SNLBitTerm*>(flattener);
}

void SNLDesign::addInstance(SNLInstance* instance) {
  if (instances_.empty()) {
    instance->id_ = 0;
  } else {
    auto it = instances_.rbegin();
    SNLInstance* lastInstance = &(*it);
    SNLID::InstanceID instanceID = lastInstance->id_+1;
    instance->id_ = instanceID;
  }
  instances_.insert(*instance);
  if (not instance->getName().empty()) {
    instanceNameIDMap_[instance->getName()] = instance->id_;
  }
}

void SNLDesign::removeInstance(SNLInstance* instance) {
  if (not instance->getName().empty()) {
    instanceNameIDMap_.erase(instance->getName());
  }
  instances_.erase(*instance);
}

SNLCollection<SNLInstance*> SNLDesign::getInstances() const {
  return SNLCollection<SNLInstance*>(new SNLIntrusiveSetCollection<SNLInstance, SNLDesignInstancesHook>(&instances_));
}

SNLCollection<SNLInstance*> SNLDesign::getSlaveInstances() const {
  return SNLCollection<SNLInstance*>(new SNLIntrusiveSetCollection<SNLInstance, SNLDesignSlaveInstancesHook>(&slaveInstances_));
}

SNLInstance* SNLDesign::getInstance(SNLID::DesignObjectID id) const {
  auto it = instances_.find(
      SNLID(SNLID::Type::Instance, getDB()->getID(), getLibrary()->getID(), getID(), 0, id, 0),
      SNLIDComp<SNLInstance>());
  if (it != instances_.end()) {
    return const_cast<SNLInstance*>(&*it);
  }
  return nullptr;
}

SNLInstance* SNLDesign::getInstance(const SNLName& name) const {
  auto it = instanceNameIDMap_.find(name);
  if (it != instanceNameIDMap_.end()) {
    SNLID::InstanceID id = it->second;
    return getInstance(id);
  }
  return nullptr;
}

void SNLDesign::addSlaveInstance(SNLInstance* instance) {
  //addSlaveInstance must be executed after addInstance.
  slaveInstances_.insert(*instance);
}

void SNLDesign::removeSlaveInstance(SNLInstance* instance) {
  slaveInstances_.erase(*instance);
}

void SNLDesign::addNet(SNLNet* net) {
  assert(dynamic_cast<SNLScalarNet*>(net) or dynamic_cast<SNLBusNet*>(net));

  if (nets_.empty()) {
    net->setID(0);
  } else {
    auto it = nets_.rbegin();
    SNLNet* lastNet = &(*it);
    SNLID::DesignObjectID netID = lastNet->getID()+1;
    net->setID(netID);
  }
  nets_.insert(*net);
  if (not net->getName().empty()) {
    netNameIDMap_[net->getName()] = net->getID();
  }
}

void SNLDesign::removeNet(SNLNet* net) {
  assert(dynamic_cast<SNLScalarNet*>(net) or dynamic_cast<SNLBusNet*>(net));

  if (not net->getName().empty()) {
    netNameIDMap_.erase(net->getName());
  }
  nets_.erase(*net);
}

SNLNet* SNLDesign::getNet(SNLID::DesignObjectID id) const {
  auto it = nets_.find(
      SNLID(SNLID::Type::Net, getDB()->getID(), getLibrary()->getID(), getID(), id, 0, 0),
      SNLIDComp<SNLNet>());
  if (it != nets_.end()) {
    return const_cast<SNLNet*>(&*it);
  }
  return nullptr;
}

SNLNet* SNLDesign::getNet(const SNLName& name) const {
  auto tit = netNameIDMap_.find(name);
  if (tit != netNameIDMap_.end()) {
    SNLID::DesignObjectID id = tit->second;
    return getNet(id);
  }
  return nullptr;
}

SNLScalarNet* SNLDesign::getScalarNet(const SNLName& name) const {
  return dynamic_cast<SNLScalarNet*>(getNet(name));
}

SNLBusNet* SNLDesign::getBusNet(const SNLName& name) const {
  return dynamic_cast<SNLBusNet*>(getNet(name));
}

SNLCollection<SNLNet*> SNLDesign::getNets() const {
  return SNLCollection<SNLNet*>(new SNLIntrusiveSetCollection<SNLNet, SNLDesignNetsHook>(&nets_));
}

SNLCollection<SNLBusNet*> SNLDesign::getBusNets() const {
  return getNets().getSubCollection<SNLBusNet*>();
}

SNLCollection<SNLScalarNet*> SNLDesign::getScalarNets() const {
  return getNets().getSubCollection<SNLScalarNet*>();
}

SNLCollection<SNLBitNet*> SNLDesign::getBitNets() const {
  auto flattener = [](const SNLBusNet* b) { return b->getBits(); };
  return getNets().getFlatCollection<SNLBusNet*, SNLBusNetBit*, SNLBitNet*>(flattener);
}

SNLDB* SNLDesign::getDB() const {
  return getLibrary()->getDB();
}

void SNLDesign::addParameter(SNLParameter* parameter) {
  parameters_.insert(*parameter);
}

void SNLDesign::removeParameter(SNLParameter* parameter) {
  parameters_.erase(*parameter);
}

SNLParameter* SNLDesign::getParameter(const SNLName& name) const {
  auto it = parameters_.find(name, SNLParameter::SNLParameterComp());
  if (it != parameters_.end()) {
    return const_cast<SNLParameter*>(&*it);
  }
  return nullptr;
}

SNLCollection<SNLParameter*> SNLDesign::getParameters() const {
  return SNLCollection<SNLParameter*>(new SNLIntrusiveSetCollection<SNLParameter, SNLDesignParametersHook>(&parameters_));
}

Card* SNLDesign::getCard() const {
  Card* card = super::getCard();
  //card->addItem(new CardDataItem<SNLID>("ID", id_));
  card->addItem(new CardDataItem<const SNLName>("Name", name_));
  card->addItem(new CardDataItem<const SNLLibrary*>("Library", library_));
  return card;
}

SNLID SNLDesign::getSNLID() const {
  return SNLID(getDB()->getID(), library_->getID(), getID());
}

bool SNLDesign::isBetween(int n, int MSB, int LSB) {
  int min = std::min(MSB, LSB);
  int max = std::max(MSB, LSB);
  return n>=min and n<=max;
}

//LCOV_EXCL_START
constexpr const char* SNLDesign::getTypeName() const {
  return "SNLDesign";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLDesign::getString() const {
  std::ostringstream str;
  if (not getLibrary()->isAnonymous()) {
    str << getLibrary()->getName().getString();
  }
  str << "(" << getLibrary()->getID() << ")";
  str << ":";
  if (not isAnonymous()) {
    str << getName().getString();
  }
  str << "(" << getID() << ")";
  return str.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLDesign::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_.getString() + " " + library_->getName().getString() + ">";  
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja