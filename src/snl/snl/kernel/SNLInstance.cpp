// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
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
#include "SNLAttributes.h"
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
    throw SNLException(reason.str());
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
    throw SNLException(reason.str());
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

bool SNLInstance::deepCompare(const SNLInstance* other, std::string& reason) const {
  if (getID() not_eq other->getID()) {
    return false; //LCOV_EXCL_LINE
  }
  if (name_ not_eq other->getName()) {
    return false; //LCOV_EXCL_LINE
  }
  //FIXME compare models: same library id, same id
  DEEP_COMPARE_MEMBER(InstParameters)
  return SNLAttributes::compareAttributes(this, other, reason);
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

SNLInstance* SNLInstance::clone(SNLDesign* design) const {
  auto newInstance = new SNLInstance(design, model_, id_, name_);
  //we can reserve instTerms_ size since we are cloning
  newInstance->instTerms_.reserve(instTerms_.size());
  newInstance->commonPostCreate();
  //clone instance parameters
  newInstance->instParameters_.clone_from(
    instParameters_,
    [newInstance](const SNLInstParameter& param){
      return new SNLInstParameter(newInstance, param.parameter_, param.value_);
    },
    [](SNLInstParameter*){} //LCOV_EXCL_LINE
  );
  SNLAttributes::cloneAttributes(this, newInstance);
  return newInstance;
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
  SNLID::Bit size = SNLUtils::getWidth(netMSB, netLSB);
  if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
    SNLID::Bit termMSB = busTerm->getMSB();
    SNLID::Bit termLSB = (termMSB<busTerm->getLSB())?termMSB+size-1:termMSB-size+1;
    setTermNet(term, termMSB, termLSB, net, netMSB, netLSB);
  } else {
    setTermNet(term, 0, 0, net, netMSB, netLSB);
  }
}

void SNLInstance::setTermNet(SNLTerm* term, SNLNet* net) {
  if (term->getWidth() not_eq net->getWidth()) {
    std::ostringstream reason;
    reason << "setTermNet only supported when term (width: " << term->getWidth() << ")"
      << " and net share same width (width: " << net->getWidth() << ")";
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

  for (const auto& sharedPathsElement: sharedPaths_) {
    sharedPathsElement.second->destroyFromInstance();
  }
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

SNLInstTerm* SNLInstance::getInstTerm(const SNLBitTerm* bitTerm) const {
  if (bitTerm == nullptr) {
    std::string reason = "SNLInstance::getInsTerm error in "
      + getName().getString() + " model: " + getModel()->getName().getString()
      + " bitTerm arg is null";
    throw SNLException(reason);
  }
  if (bitTerm->getDesign() != getModel()) {
    std::string reason = "SNLInstance::getInsTerm incoherency: "
      + getName().getString() + " model: " + getModel()->getName().getString()
      + " and " + bitTerm->getString() + " model: " + bitTerm->getDesign()->getName().getString()
      + " should be the same";
    throw SNLException(reason);
  }
  assert(bitTerm->getFlatID() < instTerms_.size());
  return instTerms_[bitTerm->getFlatID()];
}

SNLInstTerm* SNLInstance::getInstTerm(const SNLID::DesignObjectID termID) const {
  assert(termID < instTerms_.size());
  return instTerms_[termID];
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
  auto filter = [](const SNLInstTerm* it) { return it and dynamic_cast<SNLScalarTerm*>(it->getBitTerm()); };
  return NajaCollection(new NajaSTLCollection(&instTerms_)).getSubCollection(filter);
}

NajaCollection<SNLInstTerm*> SNLInstance::getInstBusTermBits() const {
  auto filter = [](const SNLInstTerm* it) { return it and dynamic_cast<SNLBusTermBit*>(it->getBitTerm()); };
  return NajaCollection(new NajaSTLCollection(&instTerms_)).getSubCollection(filter);
}

SNLSharedPath* SNLInstance::getSharedPath(const SNLSharedPath* tailSharedPath) const {
  auto it = sharedPaths_.find(tailSharedPath);
  if (it != sharedPaths_.end()) {
    return it->second;
  }
  return nullptr;
}

void SNLInstance::addSharedPath(SNLSharedPath* sharedPath) {
  //first check if not present ?
  sharedPaths_[sharedPath->getHeadSharedPath()] = sharedPath;
}

#if 0
void SNLInstance::removeSharedPath(SNLSharedPath* sharedPath) {
  auto it = sharedPaths_.find(sharedPath->getHeadSharedPath());
  if (it != sharedPaths_.end()) {
    sharedPaths_.erase(it);
  }
}
#endif

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

//support only strict mode for the moment
//previous and new interface should have same size.
//bus terms should have same msb/lsb
//In the future, we could support a more flexible interface
//if we have to support unbounded instances where model interface is
//detemine by instance context. In that case new interface should be 
//a super set of previous interface and IDs should not be used to match terms.
//In that case, anonymous terms will not be supported (in previous and new model);
void SNLInstance::setModel(SNLDesign* model) {
  if (getModel() == model) {
    return;
  }
  using TermVector = std::vector<SNLTerm*>;
  TermVector currentTerms(getModel()->getTerms().begin(), getModel()->getTerms().end());
  TermVector newTerms(model->getTerms().begin(), model->getTerms().end());
  if (currentTerms.size() != newTerms.size()) {
    throw SNLException("SNLInstance::setModel error: different number of terms");
  }

  using BitTermMap = std::map<SNLBitTerm*, SNLBitTerm*>;
  BitTermMap bitTermMap;
  for (size_t i=0; i<currentTerms.size(); ++i) {
    auto currentTerm = currentTerms[i];
    auto newTerm = newTerms[i];
    if (currentTerm->isAnonymous() != newTerm->isAnonymous()) {
      throw SNLException("SNLInstance::setModel error: anonymous contradiction");
    }
    if (not currentTerm->isAnonymous() && currentTerm->getName() != newTerms[i]->getName()) {
      throw SNLException("SNLInstance::setModel error: different term names");
    }
    if (currentTerm->getID() != newTerm->getID()) {
      throw SNLException("SNLInstance::setModel error: different term IDs");
    }
    if (currentTerm->getDirection() != newTerm->getDirection()) {
      throw SNLException("SNLInstance::setModel error: different term directions");
    }
    if (auto currentBusTerm = dynamic_cast<SNLBusTerm*>(currentTerm)) {
      if (auto newBusTerm = dynamic_cast<SNLBusTerm*>(newTerm)) {
        if (currentBusTerm->getMSB() != newBusTerm->getMSB() or currentBusTerm->getLSB() != newBusTerm->getLSB()) {
          throw SNLException("SNLInstance::setModel error: different bus term bits");
        }
        for (auto currentBit: currentBusTerm->getBits()) {
          auto newBit = newBusTerm->getBit(currentBit->getBit());
          bitTermMap[currentBit] = newBit;
        }
      } else {
        throw SNLException("SNLInstance::setModel error: different term types");
      }
    } else {
      if (auto newScalarTerm = dynamic_cast<SNLScalarTerm*>(newTerm)) {
        bitTermMap[static_cast<SNLBitTerm*>(currentTerm)] = newScalarTerm;
      } else {
        throw SNLException("SNLInstance::setModel error: different term types");
      }
    }
  }
  
  //collect parameters
  using ParametersVector = std::vector<SNLParameter*>;
  ParametersVector currentParameters(getModel()->getParameters().begin(), getModel()->getParameters().end());
  ParametersVector newParameters(model->getParameters().begin(), model->getParameters().end());
  if (currentParameters.size() != newParameters.size()) {
    throw SNLException("SNLInstance::setModel error: different number of parameters");
  }
  using ParametersMap = std::map<SNLParameter*, SNLParameter*>;
  ParametersMap parameterMap;
  for (size_t i=0; i<currentParameters.size(); ++i) {
    auto currentParameter = currentParameters[i];
    auto newParameter = newParameters[i];
    if (currentParameter->getName() != newParameter->getName()) {
      throw SNLException("SNLInstance::setModel error: different parameter names");
    }
    if (currentParameter->getType() != newParameter->getType()) {
      throw SNLException("SNLInstance::setModel error: different parameter types");
    }
    parameterMap[currentParameter] = newParameter;
  }

  for (auto instTerm: instTerms_) {
    auto currentTerm = instTerm->getBitTerm();
    auto it = bitTermMap.find(currentTerm);
    if (it == bitTermMap.end()) {
      throw SNLException("SNLInstance::setModel error: term not found in new model");
    }
    auto newTerm = it->second;
    instTerm->bitTerm_ = newTerm;
  }
  //
  //transfer instance parameters
  for (auto& instParameter: instParameters_) {
    auto currentParameter = instParameter.parameter_;
    auto it = parameterMap.find(currentParameter);
    if (it == parameterMap.end()) {
      throw SNLException("SNLInstance::setModel error: parameter not found in new model");
    }
    auto newParameter = it->second;
    instParameter.parameter_ = newParameter;
  }
  if (not model_->isPrimitive()) {
    model_->removeSlaveInstance(this);
  }
  if (not model->isPrimitive()) {
    model->addSlaveInstance(this);
  }
  model_ = model;
}

//LCOV_EXCL_START
const char* SNLInstance::getTypeName() const {
  return "SNLInstance";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstance::getString() const {
  if (isAnonymous()) {
    if (getModel()->isAssign()) {
      return "<assign: " + std::to_string(getID()) + ">";
    } else {
      return "<anonymous:" + std::to_string(getID()) + ">";
    }
  } else {
    return getName().getString();
  }
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstance::getDescription() const {
  std::ostringstream description;
  description << "<" << getTypeName();
  if (isAnonymous()) {
    if (getModel()->isAssign()) {
      description << "[assign]";
    } else {
      description << " [anonymous]";
    }
  } else {
    description << " " + name_.getString();
  }
  description << " " + std::to_string(getID());
  description << " " + design_->getName().getString();
  description << " " + model_->getName().getString();
  description << ">";
  return description.str();  
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLInstance::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive and not getInstTerms().empty()) {
    stream << std::string(indent+2, ' ') << "<instterms>" << std::endl;
    for (auto instTerm: getInstTerms()) {
      instTerm->debugDump(indent+4, recursive, stream);
    }
    stream << std::string(indent+2, ' ') << "</instterms>" << std::endl;
  }
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja