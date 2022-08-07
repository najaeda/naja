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

#include "SNLDB.h"

#include <list>
#include <iostream>

#include "SNLUniverse.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLDB::SNLDB(SNLUniverse* universe):
  universe_(universe)
{}

SNLDB::SNLDB(SNLUniverse* universe, SNLID::DBID id):
  universe_(universe),
  id_(id)
{}

SNLDB* SNLDB::create(SNLUniverse* universe) {
  preCreate(universe);
  SNLDB* db = new SNLDB(universe);
  db->postCreateAndSetID();
  return db;
}

SNLDB* SNLDB::create(SNLUniverse* universe, SNLID::DBID id) {
  preCreate(universe, id);
  SNLDB* db = new SNLDB(universe, id);
  db->postCreate();
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

void SNLDB::postCreateAndSetID() {
  super::postCreate();
  universe_->addDBAndSetID(this);
}

void SNLDB::postCreate() {
  super::postCreate();
  universe_->addDB(this);
}

void SNLDB::commonPreDrestroy() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  struct destroyLibraryFromDB {
    void operator()(SNL::SNLLibrary* library) {
      library->destroyFromParent();
    }
  };
  //First delete standard primitives
  //collect standard libraries
  using StandardLibraries = std::list<SNLLibrary*>;
  StandardLibraries standardLibraries;
  for (auto it = libraries_.begin(); it!=libraries_.end(); ++it) {
    SNLLibrary* library = &*it;
    if (library->isStandard()) {
      standardLibraries.push_back(library);
    }
  }
  for (auto library: standardLibraries) {
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
  library->id_ = nextLibraryID_++;
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

SNLLibrary* SNLDB::getLibrary(SNLID::LibraryID id) {
  auto it = libraries_.find(SNLID(getID(), id), SNLIDComp<SNLLibrary>());
  if (it != libraries_.end()) {
    return &*it;
  }
  return nullptr;
}

SNLLibrary* SNLDB::getLibrary(const SNLName& name) {
  auto it = libraryNameIDMap_.find(name);
  if (it != libraryNameIDMap_.end()) {
    SNLID::LibraryID id = it->second;
    return getLibrary(id);
  }
  return nullptr;
}

SNLCollection<SNLLibrary*> SNLDB::getLibraries() const {
  return SNLCollection(new SNLIntrusiveSetCollection(&libraries_));
}

SNLID SNLDB::getSNLID() const {
  return SNLID(id_);
}

//LCOV_EXCL_START
const char* SNLDB::getTypeName() const {
  return "SNLDB";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLDB::getString() const {
  return std::string();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLDB::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + std::to_string(getID()) + ">";  
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
