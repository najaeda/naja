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

#include "SNLLibrary.h"

#include <iostream>
#include <sstream>

#include "SNLDB.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLLibrary::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string SNLLibrary::Type::getString() const {
  switch (typeEnum_) {
    case Type::Standard: return "Standard";
    case Type::InDB0: return "InDB0";
    case Type::Primitives: return "Primitives";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

SNLLibrary::SNLLibrary(SNLDB* parent, Type type, const SNLName& name):
  super(),
  name_(name),
  type_(type),
  parent_(parent),
  isRoot_(true)
{}

SNLLibrary::SNLLibrary(SNLDB* parent, SNLID::LibraryID id, Type type, const SNLName& name):
  super(),
  id_(id),
  name_(name),
  type_(type),
  parent_(parent),
  isRoot_(true)
{}

SNLLibrary::SNLLibrary(SNLLibrary* parent, Type type, const SNLName& name):
  super(),
  name_(name),
  type_(type),
  parent_(parent),
  isRoot_(false)
{}

SNLLibrary::SNLLibrary(SNLLibrary* parent, SNLID::LibraryID id, Type type, const SNLName& name):
  super(),
  id_(id),
  name_(name),
  type_(type),
  parent_(parent),
  isRoot_(false)
{}

SNLLibrary* SNLLibrary::create(SNLDB* db, const SNLName& name) {
  preCreate(db, Type::Standard, name);
  SNLLibrary* library = new SNLLibrary(db, Type::Standard, name);
  library->postCreateAndSetID();
  return library;
}

SNLLibrary* SNLLibrary::create(SNLDB* db, Type type, const SNLName& name) {
  preCreate(db, type, name);
  SNLLibrary* library = new SNLLibrary(db, type, name);
  library->postCreateAndSetID();
  return library;
}

SNLLibrary* SNLLibrary::create(SNLDB* db, SNLID::LibraryID libraryID, const Type type, const SNLName& name) {
  preCreate(db, libraryID, type, name);
  SNLLibrary* library = new SNLLibrary(db, libraryID, type, name);
  library->postCreate();
  return library;
}

SNLLibrary* SNLLibrary::create(SNLLibrary* parent, const SNLName& name) {
  preCreate(parent, Type::Standard, name);
  SNLLibrary* library = new SNLLibrary(parent, Type::Standard, name);
  library->postCreateAndSetID();
  return library;
}

SNLLibrary* SNLLibrary::create(SNLLibrary* parent, Type type, const SNLName& name) {
  preCreate(parent, type, name);
  SNLLibrary* library = new SNLLibrary(parent, type, name);
  library->postCreateAndSetID();
  return library;
}

SNLLibrary* SNLLibrary::create(SNLLibrary* parent, SNLID::LibraryID id, Type type, const SNLName& name) {
  preCreate(parent, id, type, name);
  SNLLibrary* library = new SNLLibrary(parent, id, type, name);
  library->postCreate();
  return library;
}

void SNLLibrary::preCreate(SNLDB* db, Type type, const SNLName& name) {
  super::preCreate();
  if (not db) {
    throw SNLException("malformed SNLLibrary creator with NULL db argument");
  }
  if (not name.empty() and db->getLibrary(name)) {
    std::string reason = "SNLDB " + db->getString() + " contains already a SNLLibrary named: " + name.getString();
    throw SNLException(reason);
  }
}

void SNLLibrary::preCreate(SNLDB* db, SNLID::LibraryID libraryID, Type type, const SNLName& name) {
  preCreate(db, type, name);
  if (db->getLibrary(libraryID)) {
    std::string reason = "SNLDB " + db->getString() + " contains already a SNLLibrary with ID: " + std::to_string(libraryID);
    throw SNLException(reason);
  }
}

void SNLLibrary::preCreate(SNLLibrary* parentLibrary, Type type, const SNLName& name) {
  super::preCreate();
  if (not parentLibrary) {
    throw SNLException("malformed SNLLibrary creator with NULL parent library argument");
  }
  if (type not_eq parentLibrary->getType()) {
    throw SNLException("non compatible types in library constructor");
  }
  if (not name.empty() and parentLibrary->getLibrary(name)) {
    std::string reason = "SNLLibrary " + parentLibrary->getString() + " contains already a SNLLibrary named: " + name.getString();
    throw SNLException(reason);
  }
}

void SNLLibrary::preCreate(SNLLibrary* parentLibrary, SNLID::LibraryID id, Type type, const SNLName& name) {
  preCreate(parentLibrary, type, name);
  if (parentLibrary->getLibrary(id)) {
    std::string reason = "SNLLibrary " + parentLibrary->getString() + " contains already a SNLLibrary with ID: " + std::to_string(id);
    throw SNLException(reason);
  }
}

void SNLLibrary::postCreateAndSetID() {
  super::postCreate();
  if (isRoot()) {
    static_cast<SNLDB*>(parent_)->addLibraryAndSetID(this);
  } else {
    static_cast<SNLLibrary*>(parent_)->addLibraryAndSetID(this);
  }
}

void SNLLibrary::postCreate() {
  super::postCreate();
  if (isRoot()) {
    static_cast<SNLDB*>(parent_)->addLibrary(this);
  } else {
    static_cast<SNLLibrary*>(parent_)->addLibrary(this);
  }
}

void SNLLibrary::commonPreDestroy() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  struct destroyDesignFromLibrary {
    void operator()(SNLDesign* design) {
      design->destroyFromLibrary();
    }
  };
  designs_.clear_and_dispose(destroyDesignFromLibrary());
  struct destroyLibraryFromLibrary {
    void operator()(SNLLibrary* library) {
      library->destroyFromParent();
    }
  };
  libraries_.clear_and_dispose(destroyLibraryFromLibrary());
  super::preDestroy();
}

void SNLLibrary::destroyFromParent() {
  commonPreDestroy();
  delete this;
}

void SNLLibrary::preDestroy() {
  if (isRoot()) {
    static_cast<SNLDB*>(parent_)->removeLibrary(this);
  } else {
    static_cast<SNLLibrary*>(parent_)->removeLibrary(this);
  }
  commonPreDestroy();
}

SNLDB* SNLLibrary::getDB() const {
  if (isRoot()) {
    return static_cast<SNLDB*>(parent_);
  } else {
    return getParentLibrary()->getDB();
  }
}

SNLLibrary* SNLLibrary::getParentLibrary() const {
  if (not isRoot()) {
    return static_cast<SNLLibrary*>(parent_);
  }
  return nullptr;
}

SNLLibrary* SNLLibrary::getLibrary(SNLID::LibraryID id) const {
  auto it = libraries_.find(SNLID(getDB()->getID(), id), SNLIDComp<SNLLibrary>());
  if (it != libraries_.end()) {
    return const_cast<SNLLibrary*>(&*it);
  }
  return nullptr;
}

SNLLibrary* SNLLibrary::getLibrary(const SNLName& name) const {
  auto lit = libraryNameIDMap_.find(name);
  if (lit != libraryNameIDMap_.end()) {
    SNLID::LibraryID id = lit->second;
    return getLibrary(id);
  }
  return nullptr;
}

NajaCollection<SNLLibrary*> SNLLibrary::getLibraries() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&libraries_));
}

SNLDesign* SNLLibrary::getDesign(SNLID::DesignID id) const {
  auto it = designs_.find(SNLID(getDB()->getID(), getID(), id), SNLIDComp<SNLDesign>());
  if (it != designs_.end()) {
    return const_cast<SNLDesign*>(&*it);
  }
  return nullptr;
}

SNLDesign* SNLLibrary::getDesign(const SNLName& name) const {
  auto dit = designNameIDMap_.find(name);
  if (dit != designNameIDMap_.end()) {
    SNLID::DesignID id = dit->second;
    return getDesign(id);
  }
  return nullptr;
}

NajaCollection<SNLDesign*> SNLLibrary::getDesigns() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&designs_));
}

void SNLLibrary::addLibraryAndSetID(SNLLibrary* library) {
  if (libraries_.empty()) {
    library->id_ = 0;
  } else {
    auto it = libraries_.rbegin();
    SNLLibrary* lastLibrary = &(*it);
    library->id_ = lastLibrary->id_ ;
    ++library->id_;
  }
  addLibrary(library);
}

void SNLLibrary::addLibrary(SNLLibrary* library) {
  libraries_.insert(*library);
  if (not library->isAnonymous()) {
    libraryNameIDMap_[library->getName()] = library->id_;
  }
}

void SNLLibrary::removeLibrary(SNLLibrary* library) {
  if (not library->isAnonymous()) {
    libraryNameIDMap_.erase(library->getName());
  }
  libraries_.erase(*library);
}

void SNLLibrary::addDesignAndSetID(SNLDesign* design) {
  if (designs_.empty()) {
    design->id_ = 0;
  } else {
    auto it = designs_.rbegin();
    SNLDesign* lastDesign = &(*it);
    SNLID::DesignID designID = lastDesign->id_+1;
    design->id_ = designID;
  }
  addDesign(design);
}

void SNLLibrary::addDesign(SNLDesign* design) {
  designs_.insert(*design);
  if (not design->isAnonymous()) {
    designNameIDMap_[design->getName()] = design->id_;
  }
}

void SNLLibrary::removeDesign(SNLDesign* design) {
  if (not design->isAnonymous()) {
    designNameIDMap_.erase(design->getName());
  }
  designs_.erase(*design);
}

//LCOV_EXCL_START
const char* SNLLibrary::getTypeName() const {
  return "SNLLibrary";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLLibrary::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLLibrary::getDescription() const {
  std::ostringstream description;
  description << "<" + std::string(getTypeName());
  if (not isAnonymous()) {
    description << " " + name_.getString();
  }
  if (isPrimitives()) {
    description << " (prim)";
  }
  description << " " << getID();
  description << ">";  
  return description.str();
}
//LCOV_EXCL_STOP

SNLID SNLLibrary::getSNLID() const {
  return SNLID(getDB()->getID(), getID());
}

}} // namespace SNL // namespace naja
