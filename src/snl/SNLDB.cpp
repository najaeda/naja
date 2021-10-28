#include "SNLDB.h"

#include "SNLCommon.h"

namespace SNL {

SNLDB* SNLDB::create() {
  preCreate();
  SNLDB* db = new SNLDB();
  db->postCreate();
  return db;
}

void SNLDB::preCreate() {
  super::preCreate();
}

void SNLDB::postCreate() {
  super::postCreate();
}

void SNLDB::preDestroy() {
  struct destroyLibraryFromDB {
    void operator()(SNL::SNLLibrary* library) {
      library->destroyFromDB();
    }
  };
  libraries_.clear_and_dispose(destroyLibraryFromDB());
  libraryNameIDMap_.clear();
  super::preDestroy();
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

SNLLibrary* SNLDB::getLibrary(const SNLName& name) {
  auto iit = libraryNameIDMap_.find(name);
  if (iit != libraryNameIDMap_.end()) {
    SNLID::LibraryID id = iit->second;
    auto it = libraries_.find(SNLID(id), SNLIDComp<SNLLibrary>());
    if (it != libraries_.end()) {
      return &*it;
    }
  }
  return nullptr;
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
