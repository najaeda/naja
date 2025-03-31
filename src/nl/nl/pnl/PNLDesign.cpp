// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLDesign.h"

#include <sstream>

#include "NLDB.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "PNLBitNet.h"
#include "PNLNet.h"
#include "PNLTerm.h"
#include "SNLMacros.h"
#include "PNLScalarNet.h"

namespace naja {
namespace NL {

PNLDesign::Type::Type(const TypeEnum& typeEnum) : typeEnum_(typeEnum) {}

// LCOV_EXCL_START
std::string PNLDesign::Type::getString() const {
  switch (typeEnum_) {
    case Type::Standard:
      return "Standard";
    case Type::Blackbox:
      return "Blackbox";
    case Type::Primitive:
      return "Primitive";
  }
  return "Unknown";
}

PNLDesign::PNLDesign(NLLibrary* library, const Type& type, const NLName& name)
    : super(), name_(name), library_(library), type_(type) {}

PNLDesign::PNLDesign(NLLibrary* library, NLID::DesignID id, Type type, const NLName& name):
  super(),
  id_(id),
  name_(name),
  library_(library),
  type_(type)
{}

PNLDesign* PNLDesign::create(NLLibrary* library, const NLName& name) {
  preCreate(library, Type::Standard, name);
  PNLDesign* design = new PNLDesign(library, Type::Standard, name);
  design->postCreateAndSetID();
  return design;
}

PNLDesign* PNLDesign::create(NLLibrary* library, const Type& type, const NLName& name) {
  preCreate(library, type, name);
  PNLDesign* design = new PNLDesign(library, type, name);
  design->postCreateAndSetID();
  return design;
}

PNLDesign* PNLDesign::create(NLLibrary* library, NLID::DesignID id, Type type, const NLName& name) {
  preCreate(library, id, type, name);
  PNLDesign* design = new PNLDesign(library, id, type, name);
  design->postCreate();
  return design;
}

// This should be removed tomorrow: PNLDesignID should be NLDesign ID
void PNLDesign::postCreateAndSetID() {
  super::postCreate();
  library_->addPNLDesignAndSetID(this);
}

void PNLDesign::preCreate(const NLLibrary* library, Type type, const NLName& name) {
  super::preCreate();
  if (not library) {
    throw NLException("malformed design creator with null library");
  }
  if (type == Type::Primitive and not library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot create a primitive design";
    if (name.empty()) {
      reason << " <anonymous>";
    } else {
      reason << " named: " << name.getString();
    }
    reason << " in a non primitives library: " << library->getString();
    throw NLException(reason.str());
  }
  if (type != Type::Primitive and library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot create a non primitive design";
    if (name.empty()) {
      reason << " <anonymous>";
    } else {
      reason << " named: " << name.getString();
    }
    reason << " in a primitives library: " << library->getString();
    throw NLException(reason.str());
  }
  //test if design with same name exists in library
  if (not name.empty() and library->getPNLDesign(name)) {
    std::string reason = "NLLibrary " + library->getString() + " contains already a PNLDesign named: " + name.getString();
    throw NLException(reason);
  }
}

void PNLDesign::preCreate(const NLLibrary* library, NLID::DesignID id, Type type, const NLName& name) {
  PNLDesign::preCreate(library, type, name);
  //test if design with same id exists in library
  if (library->getPNLDesign(id)) {
    std::string reason = "NLLibrary " + library->getString() + " contains already a PNLDesign with ID: " + std::to_string(id);
    throw NLException(reason);
  }
}

NLDB* PNLDesign::getDB() const {
  return library_->getDB();
}

NLID PNLDesign::getNLID() const {
  return NLID(getDB()->getID(), library_->getID(), getID());
}

void PNLDesign::commonPreDestroy() {
#ifdef PNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  struct destroyInstanceFromDesign {
    void operator()(PNLInstance* instance) {
      instance->destroyFromDesign();
    }
  };
  instances_.clear_and_dispose(destroyInstanceFromDesign());

  if (not isPrimitive()) {
    struct destroySlaveInstanceFromModel {
      void operator()(PNLInstance* instance) {
        instance->destroyFromModel();
      }
    };
    slaveInstances_.clear_and_dispose(destroySlaveInstanceFromModel());
  }

  struct destroyTermFromDesign {
    void operator()(PNLTerm* term) {
      term->destroyFromDesign();
    }
  };
  terms_.clear_and_dispose(destroyTermFromDesign());

  struct destroyNetFromDesign {
    void operator()(PNLNet* net) {
      net->destroyFromDesign();
    }
  };
  nets_.clear_and_dispose(destroyNetFromDesign());

  // struct destroyParameterFromDesign {
  //   void operator()(PNLParameter* parameter) {
  //     parameter->destroyFromDesign();
  //   }
  // };
  // parameters_.clear_and_dispose(destroyParameterFromDesign());

  super::preDestroy();
}

void PNLDesign::destroyFromLibrary() {
  commonPreDestroy();
  delete this;
}

void PNLDesign::preDestroy() {
  if (isPrimitive()) {
    //FIXME: Error
  }
  library_->removePNLDesign(this);
  commonPreDestroy();
}

bool PNLDesign::deepCompare(const PNLDesign* other,
                            std::string& reason,
                            NLDesign::CompareType type) const {
  if (type == NLDesign::CompareType::Complete and
      (getID() not_eq other->getID())) {
    std::ostringstream oss;
    oss << "Designs mismatch between ";
    oss << getDescription() << " and " << other->getDescription();
    oss << " (ID mismatch)";
    reason = oss.str();
    return false;
  }
  if (type != NLDesign::CompareType::IgnoreIDAndName and
      (name_ not_eq other->getName())) {
    std::ostringstream oss;
    oss << "Designs mismatch between ";
    oss << getDescription() << " and " << other->getDescription();
    oss << " (name mismatch)";
    reason = oss.str();
    return false;
  }
  return true;
}

// LCOV_EXCL_START
const char* PNLDesign::getTypeName() const {
  return "PNLDesign";
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
std::string PNLDesign::getString() const {
  if (not isAnonymous()) {
    return getName().getString();
  } else {
    return "<anonymous>";
  }
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
std::string PNLDesign::getDescription() const {
  std::ostringstream stream;
  stream << "<" + std::string(getTypeName());
  if (not isAnonymous()) {
    stream << " " + getName().getString();
  }
  stream << " " << getID();
  if (not getLibrary()->isAnonymous()) {
    stream << " " << getLibrary()->getName().getString();
  }
  stream << " " << getLibrary()->getID();
  stream << ">";
  return stream.str();
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void PNLDesign::debugDump(size_t indent,
                          bool recursive,
                          std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
// LCOV_EXCL_STOP

void PNLDesign::addInstance(PNLInstance* instance) {
  instances_.insert(*instance);
  if (not instance->getName().empty()) {
    instanceNameIDMap_[instance->getName()] = instance->id_;
  }
}

void PNLDesign::addInstanceAndSetID(PNLInstance* instance) {
  if (instances_.empty()) {
    instance->id_ = 0;
  } else {
    auto it = instances_.rbegin();
    PNLInstance* lastInstance = &(*it);
    NLID::DesignObjectID instanceID = lastInstance->id_ + 1;
    instance->id_ = instanceID;
  }
  addInstance(instance);
}

PNLInstance* PNLDesign::getInstance(const NLName& name) const {
  auto it = instanceNameIDMap_.find(name);
  if (it != instanceNameIDMap_.end()) {
    NLID::DesignObjectID id = it->second;
    return getInstance(id);
  }
  return nullptr;
}

PNLInstance* PNLDesign::getInstance(NLID::DesignObjectID id) const {
  auto it = instances_.find(id, NLDesign::CompareByID<PNLInstance>());
  if (it != instances_.end()) {
    return const_cast<PNLInstance*>(&*it);
  }
  return nullptr;
}

void PNLDesign::addSlaveInstance(PNLInstance* instance) {
  // addSlaveInstance must be executed after addInstance.
  slaveInstances_.insert(*instance);
}

void PNLDesign::removeSlaveInstance(PNLInstance* instance) {
  slaveInstances_.erase(*instance);
}

void PNLDesign::removeInstance(PNLInstance* instance) {
  if (not instance->getName().empty()) {
    instanceNameIDMap_.erase(instance->getName());
  }
  instances_.erase(*instance);
}

NLID::DesignReference PNLDesign::getReference() const {
  return NLID::DesignReference(getDB()->getID(), library_->getID(), getID());
}

NajaCollection<PNLTerm*> PNLDesign::getTerms() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&terms_));
}

PNLTerm* PNLDesign::getTerm(const NLName& name) const {
  auto it = termNameIDMap_.find(name);
  if (it != termNameIDMap_.end()) {
    NLID::DesignObjectID id = it->second;
    return getTerm(id);
  }
  return nullptr;
}

PNLTerm* PNLDesign::getTerm(NLID::DesignObjectID id) const {
  auto it = terms_.find(id, NLDesign::CompareByID<PNLTerm>());
  if (it != terms_.end()) {
    return const_cast<PNLTerm*>(&*it);
  }
  return nullptr;
}

PNLNet* PNLDesign::getNet(const NLName& name) const {
  auto it = netNameIDMap_.find(name);
  if (it != netNameIDMap_.end()) {
    NLID::DesignObjectID id = it->second;
    return getNet(id);
  }
  return nullptr;
}

PNLNet* PNLDesign::getNet(NLID::DesignObjectID id) const {
  auto it = nets_.find(id, NLDesign::CompareByID<PNLNet>());
  if (it != nets_.end()) {
    return const_cast<PNLNet*>(&*it);
  }
  return nullptr;
}

void PNLDesign::addNetAndSetID(PNLNet* net) {
  if (nets_.empty()) {
    net->setID(0);
  } else {
    auto it = nets_.rbegin();
    PNLNet* lastNet = &(*it);
    NLID::DesignObjectID netID = lastNet->getID() + 1;
    net->setID(netID);
  }
  addNet(net);
}

void PNLDesign::addNet(PNLNet* net) {
  nets_.insert(*net);
  if (not net->getName().empty()) {
    netNameIDMap_[net->getName()] = net->getID();
  }
}

void PNLDesign::removeNet(PNLNet* net) {
  assert(dynamic_cast<PNLScalarNet*>(net) /*or dynamic_cast<PNLBusNet*>(net)*/);

  if (not net->getName().empty()) {
    netNameIDMap_.erase(net->getName());
  }
  nets_.erase(*net);
}

PNLScalarTerm* PNLDesign::getScalarTerm(NLID::DesignObjectID id) const {
  return dynamic_cast<PNLScalarTerm*>(getTerm(id));
}

PNLScalarTerm* PNLDesign::getScalarTerm(const NLName& name) const {
  return dynamic_cast<PNLScalarTerm*>(getTerm(name));
}

OWNER_RENAME(PNLDesign, PNLTerm, termNameIDMap_)
OWNER_RENAME(PNLDesign, PNLNet, netNameIDMap_)
OWNER_RENAME(PNLDesign, PNLInstance, instanceNameIDMap_)

void PNLDesign::addTermAndSetID(PNLTerm* term) {
  if (terms_.empty()) {
    term->setID(0);
  } else {
    auto it = terms_.rbegin();
    PNLTerm* lastTerm = &(*it);
    NLID::DesignObjectID termID = lastTerm->getID() + 1;
    term->setID(termID);
  }
  addTerm(term);
}

void PNLDesign::addTerm(PNLTerm* term) {
  assert(dynamic_cast<PNLScalarTerm*>(term) /*or dynamic_cast<PNLBusTerm*>(term)*/);

  if (terms_.empty()) {
    term->setFlatID(0);
  } else {
    auto it = terms_.rbegin();
    PNLTerm* lastTerm = &(*it);
    size_t flatID = 0;
    if (PNLScalarTerm* scalarTerm = dynamic_cast<PNLScalarTerm*>(lastTerm)) {
      flatID = scalarTerm->getFlatID() + 1;
    } else {
      assert(false);
      //   PNLBusTerm* busTerm = static_cast<PNLBusTerm*>(lastTerm);
      //   flatID = busTerm->flatID_ + static_cast<size_t>(busTerm->getWidth());
    }
    term->setFlatID(flatID);
  }
  terms_.insert(*term);
  if (not term->getName().empty()) {
    termNameIDMap_[term->getName()] = term->getID();
  }

  // Create corresponding instance terminals in slave instances
  for (auto instance : getSlaveInstances()) {
    if (PNLScalarTerm* scalarTerm = dynamic_cast<PNLScalarTerm*>(term)) {
      instance->createInstTerm(scalarTerm);
    } else {
      assert(false);
      //   PNLBusTerm* busTerm = static_cast<PNLBusTerm*>(term);
      //   for (auto bit: busTerm->getBits()) {
      //     instance->createInstTerm(bit);
      //   }
    }
  }
}

void PNLDesign::removeTerm(PNLTerm* term) {
  // Remove corresponding instance terminals in slave instances
  for (auto instance : getSlaveInstances()) {
    // if (PNLBusTerm* bus = dynamic_cast<PNLBusTerm*>(term)) {
    //   for (auto bit: bus->getBits()) {
    //     instance->removeInstTerm(bit);
    //   }
    // } else {
    PNLBitTerm* bitTerm = static_cast<PNLBitTerm*>(term);
    instance->removeInstTerm(bitTerm);
    //}
  }
  if (/**dynamic_cast<PNLBusTerm*>(term) or*/ dynamic_cast<PNLScalarTerm*>(
      term)) {
    if (not term->getName().empty()) {
      termNameIDMap_.erase(term->getName());
    }
    terms_.erase(*term);
  }
}

NajaCollection<PNLInstance*> PNLDesign::getSlaveInstances() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&slaveInstances_));
}

PNLBitTerm* PNLDesign::getBitTerm(NLID::DesignObjectID id) const {
  return dynamic_cast<PNLBitTerm*>(getTerm(id));
}

PNLBitTerm* PNLDesign::getBitTerm(const NLName& name) const {
  return dynamic_cast<PNLBitTerm*>(getTerm(name));
}

NajaCollection<PNLNet*> PNLDesign::getNets() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&nets_));
}

NajaCollection<PNLBitNet*> PNLDesign::getBitNets() const {
  return getNets().getSubCollection<PNLBitNet*>();
}

NajaCollection<PNLScalarNet*> PNLDesign::getScalarNets() const {
  return getNets().getSubCollection<PNLScalarNet*>();
}

PNLScalarNet* PNLDesign::getScalarNet(NLID::DesignObjectID id) const {
  return dynamic_cast<PNLScalarNet*>(getNet(id));
}

PNLScalarNet* PNLDesign::getScalarNet(const NLName& netName) const {
  return dynamic_cast<PNLScalarNet*>(getNet(netName));
}

NajaCollection<PNLScalarTerm*> PNLDesign::getScalarTerms() const {
  return getTerms().getSubCollection<PNLScalarTerm*>();
}

NajaCollection<PNLBitTerm*> PNLDesign::getBitTerms() const {
  // auto flattener = [](const PNLBusTerm* b) { return b->getBusBits(); };
  // return getTerms().getFlatCollection<PNLBusTerm*, PNLBusTermBit*, PNLBitTerm*>(flattener);
  return getTerms().getSubCollection<PNLBitTerm*>();
}

PNLBitTerm* PNLDesign::getBitTerm(NLID::DesignObjectID id, NLID::Bit bit) const {
  PNLBitTerm* bitTerm = getScalarTerm(id);
  // if (not bitTerm) {
  //   bitTerm = getBusTermBit(id, bit);
  // }
  return bitTerm;
}

void PNLDesign::setType(Type type) {
  if (type == Type::Primitive) {
    throw NLException("cannot change design type to Primitive");
  }
  type_ = type;
}

NajaCollection<PNLInstance*> PNLDesign::getInstances() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&instances_));
}

NajaCollection<PNLInstance*> PNLDesign::getPrimitiveInstances() const {
  auto filter = [](const PNLInstance* instance) { return instance->getModel()->isPrimitive(); };
  return getInstances().getSubCollection(filter);
}

NajaCollection<PNLInstance*> PNLDesign::getNonPrimitiveInstances() const {
  auto filter = [](const PNLInstance* instance) { return not instance->getModel()->isPrimitive(); };
  return getInstances().getSubCollection(filter);
}

}  // namespace NL
}  // namespace naja