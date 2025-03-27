// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLDesign.h"

#include <sstream>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLException.h"
#include "PNLTerm.h"


namespace naja { namespace NL {

PNLDesign::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string PNLDesign::Type::getString() const {
  switch (typeEnum_) {
    case Type::Standard: return "Standard";
    case Type::Blackbox: return "Blackbox";
    case Type::Primitive: return "Primitive";
  }
  return "Unknown";
}

PNLDesign::PNLDesign(NLLibrary* library, const NLName& name):
  super(),
  name_(name),
  library_(library)
{}

PNLDesign* PNLDesign::create(NLLibrary* library, const NLName& name) {
  preCreate(library, name);
  PNLDesign* design = new PNLDesign(library, name);
  design->postCreateAndSetID();
  return design;
}

//This should be removed tomorrow: PNLDesignID should be NLDesign ID
void PNLDesign::postCreateAndSetID() {
  super::postCreate();
  library_->addPNLDesignAndSetID(this);
}

void PNLDesign::preCreate(const NLLibrary* library, const NLName& name) {
  super::preCreate();
  if (not library) {
    throw NLException("malformed design creator with null library");
  }
  //test if design with same name exists in library
  if (not name.empty() and library->getPNLDesign(name)) {
    std::string reason = "NLLibrary " + library->getString() + " contains already a PNLDesign named: " + name.getString();
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
  super::preDestroy();
}

void PNLDesign::destroyFromLibrary() {
  commonPreDestroy();
  delete this;
}

bool PNLDesign::deepCompare(
  const PNLDesign* other,
  std::string& reason,
  NLDesign::CompareType type) const {
  if (type==NLDesign::CompareType::Complete and (getID() not_eq other->getID())) {
    std::ostringstream oss;
    oss << "Designs mismatch between ";
    oss << getDescription() << " and " << other->getDescription();
    oss << " (ID mismatch)";
    reason = oss.str();
    return false;
  }
  if (type!=NLDesign::CompareType::IgnoreIDAndName and (name_ not_eq other->getName())) {
    std::ostringstream oss;
    oss << "Designs mismatch between ";
    oss << getDescription() << " and " << other->getDescription();
    oss << " (name mismatch)";
    reason = oss.str();
    return false;
  }
  return true;
}

//LCOV_EXCL_START
const char* PNLDesign::getTypeName() const {
  return "PNLDesign";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLDesign::getString() const {
  if (not isAnonymous()) {
    return getName().getString();
  } else {
    return "<anonymous>";
  }
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
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
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void PNLDesign::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

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
    NLID::DesignObjectID instanceID = lastInstance->id_+1;
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
  //addSlaveInstance must be executed after addInstance.
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
  
}} // namespace NL // namespace naja