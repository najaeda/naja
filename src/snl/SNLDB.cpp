#include "SNLDB.h"

#include "SNLUniverse.h"

namespace SNL {

SNLDB::SNLDB(SNLUniverse* universe):
  universe_(universe)
{}

SNLDB* SNLDB::create(SNLUniverse* universe) {
  preCreate();
  SNLDB* db = new SNLDB(universe);
  db->postCreate();
  return db;
}

void SNLDB::preCreate() {
  super::preCreate();
}

void SNLDB::postCreate() {
  super::postCreate();
  universe_->addDB(this);
}

void SNLDB::preDestroy() {
  SNLUniverse::get()->removeDB(this);
  struct destroyLibraryFromDB {
    void operator()(SNL::SNLLibrary* library) {
      library->destroyFromDB();
    }
  };
  libraries_.clear_and_dispose(destroyLibraryFromDB());
  libraryNameIDMap_.clear();
  super::preDestroy();
}

void SNLDB::destroyFromUniverse() {
  super::preDestroy();
  delete this;
}

void SNLDB::addLibrary(SNLLibrary* library) {
  if (libraries_.empty()) {
    library->id_ = 0;
  } else {
    auto it = libraries_.rbegin();
    SNLLibrary* lastLibrary = &(*it);
    SNLID::LibraryID libraryID = lastLibrary->id_+1;
    library->id_ = libraryID;
  }
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
  auto iit = libraryNameIDMap_.find(name);
  if (iit != libraryNameIDMap_.end()) {
    SNLID::LibraryID id = iit->second;
    return getLibrary(id);
  }
  return nullptr;
}

SNLID SNLDB::getSNLID() const {
  return SNLID(id_);
}

constexpr const char* SNLDB::getTypeName() const {
  return "SNLDB";
}

std::string SNLDB::getString() const {
  return std::string();
}

std::string SNLDB::getDescription() const {
  return "<" + std::string(getTypeName()) + ">";  
}

}
