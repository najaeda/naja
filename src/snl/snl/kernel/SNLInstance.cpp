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

#include "SNLException.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarTerm.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
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

#if 0
SNLSharedPath* SNLInstance::getSharedPath(const SNLSharedPath* tailSharedPath) const {
  auto it = sharedPaths_.find(tailSharedPath);
  if (it != sharedPaths_.end()) {
    return it->second;
  }
  return nullptr;
}

void SNLInstance::addSharedPath(const SNLSharedPath* tailSharedPath) {
}
#endif

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
  //removeInstTerm is private so following are internal errors
  assert(term->getDesign() == getModel());
  assert(term->getPositionInDesign() < instTerms_.size());
  auto instTerm = instTerms_[term->getPositionInDesign()];
  if (instTerm) {
    instTerm->destroyFromInstance();
  }
  instTerms_[term->getPositionInDesign()] = nullptr;
}

void SNLInstance::setTermsNets(const Terms& terms, const Nets& nets) {
  if (terms.size() not_eq nets.size()) {
    throw SNLException("setTermsNets error incompatible size between terms and nets");
  }
  for (size_t i=0; i<terms.size(); ++i) {
    SNLBitTerm* bitTerm = terms[i];
    assert(bitTerm);
    if (getModel() not_eq bitTerm->getDesign()) {
      throw SNLException("setTermsNets error with incompatible instance and terminal");
    }
    auto bitNet = nets[i];
    if (bitNet and bitNet->getDesign() not_eq getDesign()) {
      throw SNLException("setTermsNets error with incompatible instance and net");
    }
    SNLInstTerm* instTerm = getInstTerm(bitTerm);
    instTerm->setNet(bitNet);
  }
}

void SNLInstance::setTermNet(
  SNLTerm* term,
  SNLID::Bit termMSB, SNLID::Bit termLSB,
  SNLNet* net,
  SNLID::Bit netMSB, SNLID::Bit netLSB) {
  Terms terms;
  Nets nets;
  if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
    assert(SNLDesign::isBetween(termMSB, busTerm->getMSB(), busTerm->getLSB()));
    assert(SNLDesign::isBetween(termLSB, busTerm->getMSB(), busTerm->getLSB()));
    SNLID::Bit incr = (termMSB<termLSB)?+1:-1;
    for (SNLID::Bit bit=termMSB; (termMSB<termLSB)?bit<=termLSB:bit>=termLSB; bit+=incr) {
      terms.push_back(busTerm->getBit(bit));
    }
  } else {
    assert(termMSB == termLSB);
    auto bitTerm = static_cast<SNLBitTerm*>(term);
    terms.push_back(bitTerm);
  }
  if (auto busNet = dynamic_cast<SNLBusNet*>(net)) {
    assert(SNLDesign::isBetween(netMSB, busNet->getMSB(), busNet->getLSB()));
    assert(SNLDesign::isBetween(netLSB, busNet->getMSB(), busNet->getLSB()));
    SNLID::Bit incr = (netMSB<netLSB)?+1:-1;
    for (SNLID::Bit bit=netMSB; (netMSB<netLSB)?bit<=netLSB:bit>=netLSB; bit+=incr) {
      nets.push_back(busNet->getBit(bit));
    }
  } else {
    assert(netMSB == netLSB);
    auto bitNet = static_cast<SNLBitNet*>(net);
    nets.push_back(bitNet);
  }
  setTermsNets(terms, nets);
}

void SNLInstance::setTermNet(SNLTerm* term, SNLNet* net) {
  if (term->getSize() not_eq net->getSize()) {
    throw SNLException("setTermNet only supported when term and net share same size");
  }
  Terms terms;
  Nets nets;
  if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
    terms = Terms(busTerm->getBits().begin(), busTerm->getBits().end());
  } else {
    auto bitTerm = static_cast<SNLBitTerm*>(term);
    terms.push_back(bitTerm);
  }
  if (auto busNet = dynamic_cast<SNLBusNet*>(net)) {
    nets = Nets(busNet->getBits().begin(), busNet->getBits().end());
  } else {
    auto bitNet = static_cast<SNLBitNet*>(net);
    nets.push_back(bitNet);
  }
  setTermsNets(terms, nets);
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

SNLInstTerm* SNLInstance::getInstTerm(const SNLBitTerm* term) const {
  if (term->getDesign() != getModel()) {
    std::string reason = "SNLInstance::getInsTerm incoherency: "
      + getName().getString() + " model: " + getModel()->getName().getString()
      + " and " + term->getString() + " model: " + term->getDesign()->getName().getString()
      + " should be the same";
    throw SNLException(reason);
  }
  if (term->getPositionInDesign() > instTerms_.size()) {
    std::string reason = "SNLInstance::getInsTerm error: size issue";
    throw SNLException(reason);
  }
  return instTerms_[term->getPositionInDesign()];
}

SNLCollection<SNLInstTerm*> SNLInstance::getInstTerms() const {
  auto filter = [](const SNLInstTerm* it) { return it != nullptr; };
  return SNLCollection(new SNLSTLCollection(&instTerms_)).getSubCollection(filter);
}

SNLCollection<SNLInstTerm*> SNLInstance::getConnectedInstTerms() const {
  auto filter = [](const SNLInstTerm* it) {return it and it->getNet() != nullptr; };
  return SNLCollection(new SNLSTLCollection(&instTerms_)).getSubCollection(filter);

}

SNLCollection<SNLInstTerm*> SNLInstance::getInstScalarTerms() const {
  auto filter = [](const SNLInstTerm* it) { return it and dynamic_cast<SNLScalarTerm*>(it->getTerm()); };
  return SNLCollection(new SNLSTLCollection(&instTerms_)).getSubCollection(filter);
}

SNLCollection<SNLInstTerm*> SNLInstance::getInstBusTermBits() const {
  auto filter = [](const SNLInstTerm* it) { return it and dynamic_cast<SNLBusTermBit*>(it->getTerm()); };
  return SNLCollection(new SNLSTLCollection(&instTerms_)).getSubCollection(filter);
}

//LCOV_EXCL_START
const char* SNLInstance::getTypeName() const {
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

}} // namespace SNL // namespace naja
