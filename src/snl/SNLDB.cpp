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
  super::preDestroy();
}

void SNLDB::addLibrary(SNLLibrary* library) {
  libraries_.insert(*library);
  nameIDMap_[library->getName()] = library->getID();
}

void SNLDB::removeLibrary(SNLLibrary* library) {
  libraries_.erase(*library);
}

SNLLibrary* SNLDB::getLibrary(const SNLName& name) {
  auto iit = nameIDMap_.find(name);
  if (iit != nameIDMap_.end()) {
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
