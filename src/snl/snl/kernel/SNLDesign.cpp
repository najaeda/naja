// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesign.h"

#include <list>
#include <iostream>
#include <sstream>

#include "SNLException.h"
#include "SNLDB.h" 
#include "SNLLibrary.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLDB0.h"
#include "SNLMacros.h"

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

SNLDesign::CompareType::CompareType(const CompareTypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string SNLDesign::CompareType::getString() const {
  switch (typeEnum_) {
    case CompareType::Complete: return "Complete";
    case CompareType::IgnoreID: return "IgnoreID";
    case CompareType::IgnoreIDAndName: return "IgnoreIDAndName";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

SNLDesign* SNLDesign::create(SNLLibrary* library, const SNLName& name) {
  preCreate(library, Type::Standard, name);
  SNLDesign* design = new SNLDesign(library, Type::Standard, name);
  design->postCreateAndSetID();
  return design;
}

SNLDesign* SNLDesign::create(SNLLibrary* library, Type type, const SNLName& name) {
  preCreate(library, type, name);
  SNLDesign* design = new SNLDesign(library, type, name);
  design->postCreateAndSetID();
  return design;
}

SNLDesign* SNLDesign::create(SNLLibrary* library, SNLID::DesignID id, Type type, const SNLName& name) {
  preCreate(library, id, type, name);
  SNLDesign* design = new SNLDesign(library, id, type, name);
  design->postCreate();
  return design;
}

SNLDesign::SNLDesign(SNLLibrary* library, Type type, const SNLName& name):
  super(),
  name_(name),
  type_(type),
  library_(library)
{}

SNLDesign::SNLDesign(SNLLibrary* library, SNLID::DesignID id, Type type, const SNLName& name):
  super(),
  id_(id),
  name_(name),
  type_(type),
  library_(library)
{}

void SNLDesign::preCreate(const SNLLibrary* library, Type type, const SNLName& name) {
  super::preCreate();
  if (not library) {
    throw SNLException("malformed design creator with null library");
  }
  if (type == Type::Primitive and not library->isPrimitives()) {
    throw SNLException("non compatible types in design constructor");
  }
  if (type != Type::Primitive and library->isPrimitives()) {
    throw SNLException("non compatible types in design constructor");
  }
  //test if design with same name exists in library
  if (not name.empty() and library->getDesign(name)) {
    std::string reason = "SNLLibrary " + library->getString() + " contains already a SNLDesign named: " + name.getString();
    throw SNLException(reason);
  }
}

void SNLDesign::preCreate(const SNLLibrary* library, SNLID::DesignID id, Type type, const SNLName& name) {
  SNLDesign::preCreate(library, type, name);
  //test if design with same id exists in library
  if (library->getDesign(id)) {
    std::string reason = "SNLLibrary " + library->getString() + " contains already a SNLDesign with ID: " + std::to_string(id);
    throw SNLException(reason);
  }
}

void SNLDesign::postCreate() {
  super::postCreate();
  library_->addDesign(this);
}

void SNLDesign::postCreateAndSetID() {
  super::postCreate();
  library_->addDesignAndSetID(this);
}

void SNLDesign::commonPreDestroy() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
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

  struct destroyTermFromDesign {
    void operator()(SNLTerm* term) {
      term->destroyFromDesign();
    }
  };
  terms_.clear_and_dispose(destroyTermFromDesign());

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

void SNLDesign::addTermAndSetID(SNLTerm* term) {
 if (terms_.empty()) {
    term->setID(0);
  } else {
    auto it = terms_.rbegin();
    SNLTerm* lastTerm = &(*it);
    SNLID::DesignObjectID termID = lastTerm->getID()+1;
    term->setID(termID);
  }
  addTerm(term);
}

void SNLDesign::addTerm(SNLTerm* term) {
  assert(dynamic_cast<SNLScalarTerm*>(term) or dynamic_cast<SNLBusTerm*>(term));

  if (terms_.empty()) {
    term->setFlatID(0);
  } else {
    auto it = terms_.rbegin();
    SNLTerm* lastTerm = &(*it);
    size_t flatID = 0;
    if (SNLScalarTerm* scalarTerm = dynamic_cast<SNLScalarTerm*>(lastTerm)) {
      flatID = scalarTerm->getFlatID() + 1;
    } else {
      SNLBusTerm* busTerm = static_cast<SNLBusTerm*>(lastTerm);
      flatID = busTerm->flatID_ + static_cast<size_t>(busTerm->getSize());
    }
    term->setFlatID(flatID);
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
  auto it = terms_.find(id, CompareByID<SNLTerm>());
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

SNLScalarTerm* SNLDesign::getScalarTerm(SNLID::DesignObjectID id) const {
  return dynamic_cast<SNLScalarTerm*>(getTerm(id));
}

SNLScalarTerm* SNLDesign::getScalarTerm(const SNLName& name) const {
  return dynamic_cast<SNLScalarTerm*>(getTerm(name));
}

SNLBusTerm* SNLDesign::getBusTerm(SNLID::DesignObjectID id) const {
  return dynamic_cast<SNLBusTerm*>(getTerm(id));
}

SNLBusTerm* SNLDesign::getBusTerm(const SNLName& name) const {
  return dynamic_cast<SNLBusTerm*>(getTerm(name));
}

NajaCollection<SNLTerm*> SNLDesign::getTerms() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&terms_));
}

NajaCollection<SNLBusTerm*> SNLDesign::getBusTerms() const {
  return getTerms().getSubCollection<SNLBusTerm*>();
}

NajaCollection<SNLScalarTerm*> SNLDesign::getScalarTerms() const {
  return getTerms().getSubCollection<SNLScalarTerm*>();
}

NajaCollection<SNLBitTerm*> SNLDesign::getBitTerms() const {
  auto flattener = [](const SNLBusTerm* b) { return b->getBusBits(); };
  return getTerms().getFlatCollection<SNLBusTerm*, SNLBusTermBit*, SNLBitTerm*>(flattener);
}

void SNLDesign::addInstanceAndSetID(SNLInstance* instance) {
  if (instances_.empty()) {
    instance->id_ = 0;
  } else {
    auto it = instances_.rbegin();
    SNLInstance* lastInstance = &(*it);
    SNLID::DesignObjectID instanceID = lastInstance->id_+1;
    instance->id_ = instanceID;
  }
  addInstance(instance);
}

void SNLDesign::addInstance(SNLInstance* instance) {
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

NajaCollection<SNLInstance*> SNLDesign::getInstances() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&instances_));
}

NajaCollection<SNLInstance*> SNLDesign::getSlaveInstances() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&slaveInstances_));
}

NajaCollection<SNLInstance*> SNLDesign::getPrimitiveInstances() const {
  auto filter = [](const SNLInstance* instance) { return instance->getModel()->isPrimitive(); };
  return getInstances().getSubCollection(filter);
}

NajaCollection<SNLInstance*> SNLDesign::getNonPrimitiveInstances() const {
  auto filter = [](const SNLInstance* instance) { return not instance->getModel()->isPrimitive(); };
  return getInstances().getSubCollection(filter);
}

SNLInstance* SNLDesign::getInstance(SNLID::DesignObjectID id) const {
  auto it = instances_.find(id, CompareByID<SNLInstance>());
  if (it != instances_.end()) {
    return const_cast<SNLInstance*>(&*it);
  }
  return nullptr;
}

SNLInstance* SNLDesign::getInstance(const SNLName& name) const {
  auto it = instanceNameIDMap_.find(name);
  if (it != instanceNameIDMap_.end()) {
    SNLID::DesignObjectID id = it->second;
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

void SNLDesign::addNetAndSetID(SNLNet* net) {
  if (nets_.empty()) {
    net->setID(0);
  } else {
    auto it = nets_.rbegin();
    SNLNet* lastNet = &(*it);
    SNLID::DesignObjectID netID = lastNet->getID()+1;
    net->setID(netID);
  }
  addNet(net);
}

void SNLDesign::addNet(SNLNet* net) {
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

DESIGN_RENAME(SNLTerm, Term, termNameIDMap_)
DESIGN_RENAME(SNLNet, Net, netNameIDMap_)
DESIGN_RENAME(SNLInstance, Instance, instanceNameIDMap_)

SNLNet* SNLDesign::getNet(SNLID::DesignObjectID id) const {
  auto it = nets_.find(id, CompareByID<SNLNet>());
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

SNLScalarNet* SNLDesign::getScalarNet(SNLID::DesignObjectID id) const {
  return dynamic_cast<SNLScalarNet*>(getNet(id));
}

SNLScalarNet* SNLDesign::getScalarNet(const SNLName& name) const {
  return dynamic_cast<SNLScalarNet*>(getNet(name));
}

SNLBusNet* SNLDesign::getBusNet(SNLID::DesignObjectID id) const {
  return dynamic_cast<SNLBusNet*>(getNet(id));
}

SNLBusNet* SNLDesign::getBusNet(const SNLName& name) const {
  return dynamic_cast<SNLBusNet*>(getNet(name));
}

SNLBusNetBit* SNLDesign::getBusNetBit(SNLID::DesignObjectID id, SNLID::Bit bit) const {
  auto bus = getBusNet(id);
  if (bus) {
    return bus->getBit(bit);
  }
  return nullptr;
}

NajaCollection<SNLNet*> SNLDesign::getNets() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&nets_));
}

NajaCollection<SNLBusNet*> SNLDesign::getBusNets() const {
  return getNets().getSubCollection<SNLBusNet*>();
}

NajaCollection<SNLScalarNet*> SNLDesign::getScalarNets() const {
  return getNets().getSubCollection<SNLScalarNet*>();
}

NajaCollection<SNLBitNet*> SNLDesign::getBitNets() const {
  auto flattener = [](const SNLBusNet* b) { return b->getBusBits(); };
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

NajaCollection<SNLParameter*> SNLDesign::getParameters() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&parameters_));
}

bool SNLDesign::isTopDesign() const {
  return getDB()->getTopDesign() == this; 
}


bool SNLDesign::deepCompare(
      const SNLDesign* other,
      std::string& reason,
      CompareType type) const {
  if (type==CompareType::Complete and (getID() not_eq other->getID())) {
    return false;
  }
  if (type!=CompareType::IgnoreIDAndName and (name_ not_eq other->getName())) {
    return false;
  }
  if (type_ not_eq other->getType()) {
    return false;
  }
  DEEP_COMPARE_MEMBER(Parameters)
  DEEP_COMPARE_MEMBER(Instances)
  return true;
}

SNLID SNLDesign::getSNLID() const {
  return SNLID(getDB()->getID(), library_->getID(), getID());
}

SNLID::DesignReference SNLDesign::getReference() const {
  return SNLID::DesignReference(getDB()->getID(), library_->getID(), getID());
}

bool SNLDesign::isBetween(int n, int MSB, int LSB) {
  int min = std::min(MSB, LSB);
  int max = std::max(MSB, LSB);
  return n>=min and n<=max;
}

void SNLDesign::mergeAssigns() {
  using Instances = std::list<SNLInstance*>;
  auto filter = [](const SNLInstance* it) { return SNLDB0::isAssign(it->getModel()); };
  Instances assignInstances(
      getInstances().getSubCollection(filter).begin(),
      getInstances().getSubCollection(filter).end());
  auto assignInput = SNLDB0::getAssignInput();
  auto assignOutput = SNLDB0::getAssignOutput(); 
  for (auto assignInstance: assignInstances) {
    auto assignInstanceInput = assignInstance->getInstTerm(assignInput);
    auto assignInstanceOutput = assignInstance->getInstTerm(assignOutput);
    auto assignInputNet = assignInstanceInput->getNet();
    auto assignOutputNet = assignInstanceOutput->getNet();
    //take all components for assignOutputNet and assign them to assignInputNet
    assignOutputNet->connectAllComponentsTo(assignInputNet);
    if (dynamic_cast<SNLScalarNet*>(assignOutputNet)) {
      assignOutputNet->destroy();
    }
  }
  for (auto assignInstance: assignInstances) {
    assignInstance->destroy();
  }
}

SNLDesign* SNLDesign::cloneInterfaceToLibrary(SNLLibrary* library, const SNLName& name) const {
  if (isPrimitive()) {
    throw SNLException("cloneToLibrary cannot be called on primitive designs");
  }
  auto newDesign = SNLDesign::create(library, Type::Standard, name);
  newDesign->terms_.clone_from(
    terms_,
    [newDesign](const SNLTerm& term){
      return term.clone(newDesign);
    },
    [](SNLTerm*){} //LCOV_EXCL_LINE
  );
  newDesign->termNameIDMap_ = termNameIDMap_;
  //clone parameters
  newDesign->parameters_.clone_from(
    parameters_,
    [newDesign](const SNLParameter& parameter){ 
      return new SNLParameter(newDesign, parameter.name_, parameter.type_, parameter.value_);
    },
    [](SNLParameter*){} //LCOV_EXCL_LINE
  );
  return newDesign;
}

SNLDesign* SNLDesign::cloneInterface(const SNLName& name) const {
  return cloneInterfaceToLibrary(getLibrary(), name);
}

SNLDesign* SNLDesign::cloneToLibrary(SNLLibrary* library, const SNLName& name) const {
  if (not name.empty() and library->getDesign(name)) {
    std::string reason = "SNLLibrary " + library->getString() + " contains already a SNLDesign named: " + getName().getString();
    throw SNLException(reason);
  }

  //start with interface uniquification
  auto newDesign = cloneInterfaceToLibrary(library, name);
  //clone instances
  newDesign->instances_.clone_from(
    instances_,
    [newDesign](const SNLInstance& instance){ return instance.clone(newDesign); },
    [](SNLInstance*){} //LCOV_EXCL_LINE
  );
  newDesign->instanceNameIDMap_ = instanceNameIDMap_;

  //clone nets
  newDesign->nets_.clone_from(
    nets_,
    [newDesign](const SNLNet& net){ return net.clone(newDesign); },
    [](SNLNet*){} //LCOV_EXCL_LINE
  );
  newDesign->netNameIDMap_ = netNameIDMap_;

  return newDesign;
}

SNLDesign* SNLDesign::clone(const SNLName& name) const {
  return cloneToLibrary(getLibrary(), name);
}

//LCOV_EXCL_START
const char* SNLDesign::getTypeName() const {
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
  std::ostringstream stream;
  stream << "<" + std::string(getTypeName());
  if (not isAnonymous()) {
    stream << " " + getName().getString();
  }
  stream << " " << getID();
  if (isPrimitive()) {
    stream << " (prim)";
  }
  if (not getLibrary()->isAnonymous()) {
    stream << " " << getLibrary()->getName().getString();
  }
  stream << " " << getLibrary()->getID();
  stream << ">";
  return stream.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLDesign::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    if (not getTerms().empty()) {
      stream << std::string(indent+2, ' ') << "<terms>" << std::endl;
      for (auto term: getTerms()) {
        term->debugDump(indent+4, false, stream);
      }
      stream << std::string(indent+2, ' ') << "</terms>" << std::endl;
    }
    if (not getNets().empty()) {
      stream << std::string(indent+2, ' ') << "<nets>" << std::endl;
      for (auto net: getNets()) {
        net->debugDump(indent+4, recursive, stream);
      }
      stream << std::string(indent+2, ' ') << "</nets>" << std::endl;
    }
    if (not getInstances().empty()) {
      stream << std::string(indent+2, ' ') << "<instances>" << std::endl;
      for (auto instance: getInstances()) {
        instance->debugDump(indent+4, recursive, stream);
      }
      stream << std::string(indent+2, ' ') << "</instances>" << std::endl;
    }
  }
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
