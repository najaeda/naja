// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDB.h"

#include <list>
#include <iostream>
#include <sstream>

#include "SNLUniverse.h"
#include "SNLDB0.h"
#include "SNLException.h"
#include "SNLMacros.h"

namespace naja { namespace SNL {

SNLDB::SNLDB(SNLID::DBID id):
  id_(id)
{}

SNLDB* SNLDB::create(SNLUniverse* universe) {
  preCreate(universe);
  SNLDB* db = new SNLDB();
  db->postCreateAndSetID(universe);
  return db;
}

SNLDB* SNLDB::create(SNLUniverse* universe, SNLID::DBID id) {
  preCreate(universe, id);
  SNLDB* db = new SNLDB(id);
  db->postCreate(universe);
  return db;
}

void SNLDB::preCreate(SNLUniverse* universe) {
  super::preCreate();
  if (not universe) {
    throw SNLException("DB creation: NULL Universe");
  }
}

void SNLDB::preCreate(SNLUniverse* universe, SNLID::DBID id) {
  preCreate(universe);
  if (SNLUniverse::get()->getDB(id)) {
    throw SNLException("DB collision");
  }
}

void SNLDB::postCreateAndSetID(SNLUniverse* universe) {
  super::postCreate();
  universe->addDBAndSetID(this);
}

void SNLDB::postCreate(SNLUniverse* universe) {
  super::postCreate();
  universe->addDB(this);
}

void SNLDB::commonPreDrestroy() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  struct destroyLibraryFromDB {
    void operator()(SNL::SNLLibrary* library) {
      library->destroyFromDB();
    }
  };
  //First delete standard primitives
  //collect standard libraries
  using Libraries = std::list<SNLLibrary*>;
  Libraries nonRootLibraries;
  Libraries standardRootLibraries;
  for (auto it = libraries_.begin(); it!=libraries_.end(); ++it) {
    SNLLibrary* library = &*it;
    //destroy first root and standard library 
    if (not library->isRoot()) {
      nonRootLibraries.push_back(library);
    } else if (library->isStandard()) {
      standardRootLibraries.push_back(library);
    } 
  }
  for (auto library: nonRootLibraries) {
    libraries_.erase(*library);
  }
  for (auto library: standardRootLibraries) {
    libraries_.erase_and_dispose(*library, destroyLibraryFromDB());
  }

  //remove all the rest
  libraries_.clear_and_dispose(destroyLibraryFromDB());
  libraryNameIDMap_.clear();
  super::preDestroy();
}

void SNLDB::preDestroy() {
  commonPreDrestroy();
  SNLUniverse::get()->removeDB(this);
}

void SNLDB::destroyFromUniverse() {
  commonPreDrestroy();
  delete this;
}

void SNLDB::addLibraryAndSetID(SNLLibrary* library) {
  if (libraries_.empty()) {
    library->id_ = 0;
  } else {
    auto it = libraries_.rbegin();
    SNLLibrary* lastLibrary = &(*it);
    library->id_ = lastLibrary->id_;
    ++library->id_;
  }
  addLibrary(library);
}

void SNLDB::addLibrary(SNLLibrary* library) {
  libraries_.insert(*library);
  libraryNameIDMap_[library->getName()] = library->id_;
}

void SNLDB::removeLibrary(SNLLibrary* library) {
  libraries_.erase(*library);
  libraryNameIDMap_.erase(library->getName());
}

OWNER_RENAME(SNLDB, SNLLibrary, libraryNameIDMap_) 

SNLLibrary* SNLDB::getLibrary(SNLID::LibraryID id) const {
  auto it = libraries_.find(SNLID(getID(), id), SNLIDComp<SNLLibrary>());
  if (it != libraries_.end()) {
    return const_cast<SNLLibrary*>(&*it);
  }
  return nullptr;
}

SNLLibrary* SNLDB::getLibrary(const SNLName& name) const {
  auto it = libraryNameIDMap_.find(name);
  if (it != libraryNameIDMap_.end()) {
    SNLID::LibraryID id = it->second;
    return getLibrary(id);
  }
  return nullptr;
}

SNLDesign* SNLDB::getDesign(const SNLID::DBDesignReference& designReference) const {
  auto library = getLibrary(designReference.libraryID_);
  if (library) {
    return library->getSNLDesign(designReference.designID_);
  }
  return nullptr;
}

SNLDesign* SNLDB::getDesign(const SNLName& name) const {
  for (auto library: getLibraries()) {
    auto design = library->getSNLDesign(name);
    if (design) {
      return design;
    }
  }
  return nullptr;
}

NajaCollection<SNLLibrary*> SNLDB::getGlobalLibraries() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&libraries_));
}

NajaCollection<SNLLibrary*> SNLDB::getPrimitiveLibraries() const {
  auto filter = [](const SNLLibrary* l) { return l->isPrimitives(); };
  return getGlobalLibraries().getSubCollection(filter);
}

NajaCollection<SNLLibrary*> SNLDB::getLibraries() const {
  auto filter = [](const SNLLibrary* l) { return l->isRoot(); };
  return getGlobalLibraries().getSubCollection(filter);
}

SNLID SNLDB::getSNLID() const {
  return SNLID(id_);
}

void SNLDB::setID(SNLID::DBID id) {
  if (SNLUniverse::get()->isDB0(this)) {
    //FIXME
    //error
  }
  SNLUniverse::get()->removeDB(this);
  id_ = id;
  SNLUniverse::get()->addDB(this);
}

bool SNLDB::isTopDB() const {
  return SNLUniverse::get()->getTopDB() == this;
}

SNLDesign* SNLDB::getTopDesign() const {
  return topDesign_;
}

void SNLDB::setTopDesign(SNLDesign* design) {
  if (design and design->getDB() not_eq this) {
    std::ostringstream reason;
    reason << "Impossible setTopDesign call with ";
    reason << design->getString() << " and ";
    reason << getString();
    throw SNLException(reason.str());
  }
  topDesign_ = design;
}

bool SNLDB::deepCompare(const SNLDB* other, std::string& reason) const {
  //don't compare SNLDB ID
  DEEP_COMPARE_MEMBER(Libraries)
  return true;
}

void SNLDB::mergeAssigns() {
  for (auto library: getLibraries()) {
    if (not library->isPrimitives()) {
      library->mergeAssigns();
    }
  }
}

//LCOV_EXCL_START
const char* SNLDB::getTypeName() const {
  return "SNLDB";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLDB::getString() const {
  return "<" + std::string(getTypeName()) + " " + std::to_string(getID()) + ">";  
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLDB::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + std::to_string(getID()) + ">";  
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLDB::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    for (auto lib: getLibraries()) {
      lib->debugDump(indent+2, recursive, stream);
    }
  }
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
