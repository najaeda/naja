#include "SNLUniverse.h"

namespace SNL {

SNLUniverse* SNLUniverse::universe_ = nullptr;

SNLUniverse* SNLUniverse::create() {
  preCreate();
  universe_ = new SNLUniverse();
  universe_->postCreate();
  return universe_;
}

void SNLUniverse::preCreate() {
  super::preCreate();
  //FIXME: verify that no other Universe exists
}

void SNLUniverse::postCreate() {
  super::postCreate();
}

void SNLUniverse::preDestroy() {
  struct destroyDBFromUniverse {
    void operator()(SNL::SNLDB* db) {
      db->destroyFromUniverse();
    }
  };
  dbs_.clear_and_dispose(destroyDBFromUniverse());
  super::preDestroy();
}

SNLUniverse* SNLUniverse::get() {
  return universe_;
}

void SNLUniverse::addDB(SNLDB* db) {
  if (dbs_.empty()) {
    db->id_ = 0;
  } else {
    auto it = dbs_.rbegin();
    SNLDB* lastDB = &(*it);
    SNLID::DBID dbID = lastDB->id_+1;
    db->id_ = dbID;
  }
  dbs_.insert(*db);
}

void SNLUniverse::removeDB(SNLDB* db) {
  dbs_.erase(*db);
}

constexpr const char* SNLUniverse::getTypeName() const {
  return "SNLUniverse";
}

std::string SNLUniverse::getString() const {
  return std::string();
}

std::string SNLUniverse::getDescription() const {
  return "<" + std::string(getTypeName()) + ">";  
}


}
