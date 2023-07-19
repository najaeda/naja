// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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
#include "SNLUtils.h"
#include "SNLMacros.h"

namespace {

//LCOV_EXCL_START
void printTerms(const naja::SNL::SNLInstance::Terms& terms, std::ostream& stream) {
  stream << "[";
  bool first = true;
  for (auto term: terms) {
    if (not first) {
      stream << ", ";
    }
    first = false;
    if (term) {
      stream << term->getString();
    } else {
      stream << "null";
    }
  }
  stream << "]";
}
//LCOV_EXCL_STOP

}

namespace naja { namespace SNL {

SNLInstance::SNLInstance(SNLDesign* design, SNLDesign* model, const SNLName& name):
  super(),
  design_(design),
  model_(model),
  name_(name)
{}

SNLInstance::SNLInstance(SNLDesign* design, SNLDesign* model, SNLID::DesignObjectID id, const SNLName& name):
  super(),
  design_(design),
  model_(model),
  id_(id),
  name_(name)
{}

SNLInstance* SNLInstance::create(SNLDesign* design, SNLDesign* model, const SNLName& name) {
  preCreate(design, model, name);
  SNLInstance* instance = new SNLInstance(design, model, name);
  instance->postCreateAndSetID();
  return instance;
}

SNLInstance* SNLInstance::create(SNLDesign* design, SNLDesign* model, SNLID::DesignObjectID id, const SNLName& name) {
  preCreate(design, model, id, name);
  SNLInstance* instance = new SNLInstance(design, model, id, name);
  instance->postCreate();
  return instance;
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

void SNLInstance::preCreate(SNLDesign* design, const SNLDesign* model, SNLID::DesignObjectID id, const SNLName& name) {
  preCreate(design, model, name);
  if (design->getInstance(id)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLInstance with id: " + std::to_string(id);
    throw SNLException(reason);
  }
}

void SNLInstance::commonPostCreate() {
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

void SNLInstance::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addInstanceAndSetID(this);
  commonPostCreate();

}

void SNLInstance::postCreate() {
  super::postCreate();
  getDesign()->addInstance(this);
  commonPostCreate();
}

void SNLInstance::createInstTerm(SNLBitTerm* term) {
  instTerms_.push_back(SNLInstTerm::create(this, term));
}

void SNLInstance::removeInstTerm(SNLBitTerm* term) {
  //removeInstTerm is private so following are internal errors
  assert(term->getDesign() == getModel());
  assert(term->getFlatID() < instTerms_.size());
  auto instTerm = instTerms_[term->getFlatID()];
  if (instTerm) {
    instTerm->destroyFromInstance();
  }
  instTerms_[term->getFlatID()] = nullptr;
}

DESIGN_OBJECT_SET_NAME(SNLInstance, Instance, instance)

void SNLInstance::setTermsNets(const Terms& terms, const Nets& nets) {
  if (terms.size() not_eq nets.size()) {
    std::ostringstream reason;
    reason << "setTermsNets only supported when terms (size: " << terms.size() << " ";
    printTerms(terms, reason);
    reason << ") and nets share same size (size: " << nets.size() << ")";
    throw SNLException(reason.str());
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

void SNLInstance::setTermNet(
  SNLTerm* term,
  SNLNet* net,
  SNLID::Bit netMSB, SNLID::Bit netLSB) {
  SNLID::Bit size = SNLUtils::getSize(netMSB, netLSB);
  if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
    SNLID::Bit termMSB = busTerm->getMSB();
    SNLID::Bit termLSB = (termMSB<busTerm->getLSB())?termMSB+size-1:termMSB-size+1;
    setTermNet(term, termMSB, termLSB, net, netMSB, netLSB);
  } else {
    setTermNet(term, 0, 0, net, netMSB, netLSB);
  }
}

void SNLInstance::setTermNet(SNLTerm* term, SNLNet* net) {
  if (term->getSize() not_eq net->getSize()) {
    std::ostringstream reason;
    reason << "setTermNet only supported when term (size: " << term->getSize() << ")"
      << " and net share same size (size: " << net->getSize() << ")";
    throw SNLException(reason.str());
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

  struct destroySharedPathFromInstance {
    void operator()(SNLSharedPath* path) {
      path->destroyFromInstance();
    }
  };
  sharedPaths_.clear_and_dispose(destroySharedPathFromInstance());
  for (auto instTerm: instTerms_) {
    if (instTerm) {
      instTerm->destroyFromInstance();
    }
  }
  struct destroyInstParameterFromInstance {
    void operator()(SNLInstParameter* instParameter) {
      instParameter->destroyFromInstance();
    }
  };
  instParameters_.clear_and_dispose(destroyInstParameterFromInstance());
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

SNLID::DesignObjectReference SNLInstance::getReference() const {
  return SNLID::DesignObjectReference(getDesign()->getReference(), getID());
}


bool SNLInstance::isBlackBox() const {
  return getModel()->isBlackBox();
}

bool SNLInstance::isPrimitive() const {
  return getModel()->isPrimitive();
}

bool SNLInstance::isLeaf() const {
  return getModel()->isLeaf();
}

SNLInstTerm* SNLInstance::getInstTerm(const SNLBitTerm* term) const {
  if (term->getDesign() != getModel()) {
    std::string reason = "SNLInstance::getInsTerm incoherency: "
      + getName().getString() + " model: " + getModel()->getName().getString()
      + " and " + term->getString() + " model: " + term->getDesign()->getName().getString()
      + " should be the same";
    throw SNLException(reason);
  }
  assert(term->getFlatID() < instTerms_.size());
  return instTerms_[term->getFlatID()];
}

NajaCollection<SNLInstTerm*> SNLInstance::getInstTerms() const {
  auto filter = [](const SNLInstTerm* it) { return it != nullptr; };
  return NajaCollection(new NajaSTLCollection(&instTerms_)).getSubCollection(filter);
}

NajaCollection<SNLInstTerm*> SNLInstance::getConnectedInstTerms() const {
  auto filter = [](const SNLInstTerm* it) {return it and it->getNet() != nullptr; };
  return NajaCollection(new NajaSTLCollection(&instTerms_)).getSubCollection(filter);

}

NajaCollection<SNLInstTerm*> SNLInstance::getInstScalarTerms() const {
  auto filter = [](const SNLInstTerm* it) { return it and dynamic_cast<SNLScalarTerm*>(it->getTerm()); };
  return NajaCollection(new NajaSTLCollection(&instTerms_)).getSubCollection(filter);
}

NajaCollection<SNLInstTerm*> SNLInstance::getInstBusTermBits() const {
  auto filter = [](const SNLInstTerm* it) { return it and dynamic_cast<SNLBusTermBit*>(it->getTerm()); };
  return NajaCollection(new NajaSTLCollection(&instTerms_)).getSubCollection(filter);
}

SNLSharedPath* SNLInstance::getSharedPath(const SNLSharedPath* sharedPath) const {
  //SharedPath: [HeadPath*, TailInstance*]
  //SharedPaths are stored in TailInstance with key HeadPath->getHeadInstance()->getSNLID()
  //Single instance shared path: [Null, TailInstance*] is stored with key max SNLID 
  auto key = getSNLID();
  if (sharedPath) {
    key = sharedPath->getSNLID();
  }
  auto it = sharedPaths_.find(key, SNLIDComp<SNLSharedPath>());
  if (it != sharedPaths_.end()) {
    return const_cast<SNLSharedPath*>(&*it);
  }
  return nullptr;
}

void SNLInstance::addSharedPath(SNLSharedPath* sharedPath) {
  sharedPaths_.insert(*sharedPath);
}

void SNLInstance::removeSharedPath(SNLSharedPath* sharedPath) {
  sharedPaths_.erase(*sharedPath);
}

void SNLInstance::addInstParameter(SNLInstParameter* instParameter) {
  instParameters_.insert(*instParameter);
}

void SNLInstance::removeInstParameter(SNLInstParameter* instParameter) {
  instParameters_.erase(*instParameter);
}

SNLInstParameter* SNLInstance::getInstParameter(const SNLName& name) const {
  auto it = instParameters_.find(name, SNLNameComp<SNLInstParameter>());
  if (it != instParameters_.end()) {
    return const_cast<SNLInstParameter*>(&*it);
  }
  return nullptr;
}

NajaCollection<SNLInstParameter*> SNLInstance::getInstParameters() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&instParameters_));
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
