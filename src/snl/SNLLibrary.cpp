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
  auto it = libraries_.find(name, SNLNameComp<SNLLibrary>());
  if (it != libraries_.end()) {
    return &*it;
  }
  return nullptr;
}

SNLCollection<SNLLibrary> SNLLibrary::getLibraries() {
  return SNLCollection<SNLLibrary>(new SNLIntrusiveSetCollection<SNLLibrary, SNLLibraryLibrariesHook>(&libraries_));
}

SNLCollection<SNLDesign> SNLLibrary::getDesigns() {
  return SNLCollection<SNLDesign>(new SNLIntrusiveSetCollection<SNLDesign, SNLLibraryDesignsHook>(&designs_));
}

void SNLLibrary::addLibrary(SNLLibrary* library) {
  libraries_.insert(*library);
}

void SNLLibrary::removeLibrary(SNLLibrary* library) {
  libraries_.erase(*library);
}

void SNLLibrary::addDesign(SNLDesign* design) {
  designs_.insert(*design);
}

void SNLLibrary::removeDesign(SNLDesign* design) {
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

}
