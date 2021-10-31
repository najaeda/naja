#include "SNLLibrary.h"

#include "SNLDB.h"
#include "SNLCommon.h"

namespace SNL {

SNLLibrary::SNLLibrary(SNLDB* parent, const SNLName& name):
  super(),
  name_(name),
  parent_(parent),
  isRootLibrary_(true)
{}

SNLLibrary::SNLLibrary(SNLLibrary* parent, const SNLName& name):
  super(),
  name_(name),
  parent_(parent),
  isRootLibrary_(false)
{}

SNLLibrary* SNLLibrary::create(SNLDB* parent, const SNLName& name) {
  preCreate(parent, name);
  SNLLibrary* library = new SNLLibrary(parent, name);
  library->postCreate();
  return library;
}

SNLLibrary* SNLLibrary::create(SNLLibrary* parent, const SNLName& name) {
  preCreate(parent, name);
  SNLLibrary* library = new SNLLibrary(parent, name);
  library->postCreate();
  return library;
}

void SNLLibrary::preCreate(const SNLDB* parent, const SNLName& name) {
  super::preCreate();
}

void SNLLibrary::preCreate(const SNLLibrary* parent, const SNLName& name) {
  super::preCreate();
}

void SNLLibrary::postCreate() {
  super::postCreate();
  if (isRootLibrary()) {
    static_cast<SNLDB*>(parent_)->addLibrary(this);
  } else {
    static_cast<SNLLibrary*>(parent_)->addLibrary(this);
  }
}

void SNLLibrary::destroyFromDB() {
  super::preDestroy();
  delete this;
}

void SNLLibrary::preDestroy() {
  if (isRootLibrary()) {
    static_cast<SNLDB*>(parent_)->removeLibrary(this);
  } else {
    static_cast<SNLLibrary*>(parent_)->removeLibrary(this);
  }
  struct destroyDesignFromLibrary {
    void operator()(SNLDesign* design) {
      design->destroyFromLibrary();
    }
  };
  designs_.clear_and_dispose(destroyDesignFromLibrary());
  super::preDestroy();
}

SNLDB* SNLLibrary::getDB() const {
  if (isRootLibrary()) {
    return static_cast<SNLDB*>(parent_);
  } else {
    return getParentLibrary()->getDB();
  }
}

SNLLibrary* SNLLibrary::getParentLibrary() const {
  if (not isRootLibrary()) {
    return static_cast<SNLLibrary*>(parent_);
  }
  return nullptr;
}

SNLLibrary* SNLLibrary::getLibrary(const SNLName& name) {
  auto lit = libraryNameIDMap_.find(name);
  if (lit != libraryNameIDMap_.end()) {
    SNLID::LibraryID id = lit->second;
    auto it = libraries_.find(SNLID(getDB()->getID(), id), SNLIDComp<SNLLibrary>());
    if (it != libraries_.end()) {
      return &*it;
    }
  }
  return nullptr;
}

SNLCollection<SNLLibrary> SNLLibrary::getLibraries() {
  return SNLCollection<SNLLibrary>(new SNLIntrusiveSetCollection<SNLLibrary, SNLLibraryLibrariesHook>(&libraries_));
}

SNLDesign* SNLLibrary::getDesign(const SNLName& name) {
  auto dit = designNameIDMap_.find(name);
  if (dit != designNameIDMap_.end()) {
    SNLID::DesignObjectID id = dit->second;
    auto it = designs_.find(SNLID(getID(), id), SNLIDComp<SNLDesign>());
    if (it != designs_.end()) {
      return &*it;
    }
  }
  return nullptr;
}

SNLCollection<SNLDesign> SNLLibrary::getDesigns() {
  return SNLCollection<SNLDesign>(new SNLIntrusiveSetCollection<SNLDesign, SNLLibraryDesignsHook>(&designs_));
}

void SNLLibrary::addLibrary(SNLLibrary* library) {
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

void SNLLibrary::removeLibrary(SNLLibrary* library) {
  libraries_.erase(*library);
}

void SNLLibrary::addDesign(SNLDesign* design) {
  if (designs_.empty()) {
    design->id_ = 0;
  } else {
    auto it = designs_.rbegin();
    SNLDesign* lastDesign = &(*it);
    SNLID::DesignID designID = lastDesign->id_+1;
    design->id_ = designID;
  }
  designs_.insert(*design);
  if (not design->getName().empty()) {
    designNameIDMap_[design->getName()] = design->id_;
  }
}

void SNLLibrary::removeDesign(SNLDesign* design) {
  if (not design->getName().empty()) {
    designNameIDMap_.erase(design->getName());
  }
  designs_.erase(*design);
}

constexpr const char* SNLLibrary::getTypeName() const {
  return "SNLLibrary";
}

std::string SNLLibrary::getString() const {
  return std::string();
}

std::string SNLLibrary::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_ + ">";  
}

SNLID SNLLibrary::getSNLID() const {
  return SNLID(getDB()->getID(), getID());
}

}
