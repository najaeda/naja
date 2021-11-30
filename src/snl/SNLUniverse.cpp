#include "SNLUniverse.h"

#include "SNLDB0.h"

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
  //create the special DB0 which holds the SNL managed libraries
  //such as assign cell
  db0_ = SNLDB0::create(this);
}

void SNLUniverse::preDestroy() {
  struct destroyDBFromUniverse {
    void operator()(SNL::SNLDB* db) {
      db->destroyFromUniverse();
    }
  };
  dbs_.clear_and_dispose(destroyDBFromUniverse());
  universe_ = nullptr;
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
    SNLID::DBID dbID = lastDB->id_;
    ++dbID;
    db->id_ = dbID;
  }
  dbs_.insert(*db);
}

void SNLUniverse::removeDB(SNLDB* db) {
  dbs_.erase(*db);
}

bool SNLUniverse::isDB0(const SNLDB* db) {
  auto universe = get();
  if (universe) {
    return db == universe->db0_;
  }
  return false;
}

SNLDesign* SNLUniverse::getAssign() {
  auto universe = get();
  if (universe) {
    return universe->assign_;
  }
  return nullptr;
}

SNLScalarTerm* SNLUniverse::getAssignInput() {
  auto universe = get();
  if (universe) {
    return universe->assignInput_;
  }
  return nullptr;
}

SNLScalarTerm* SNLUniverse::getAssignOutput() {
  auto universe = get();
  if (universe) {
    return universe->assignOutput_;
  }
  return nullptr;
}

SNLCollection<SNLDB*> SNLUniverse::getDBs() const {
  return SNLCollection<SNLDB*>(new SNLIntrusiveConstSetCollection<SNLDB, SNLUniverseDBsHook>(&dbs_));
}

SNLDB* SNLUniverse::getDB(SNLID::DBID id) {
  auto it = dbs_.find(SNLID(id), SNLIDComp<SNLDB>());
  if (it != dbs_.end()) {
    return &*it;
  }
  return nullptr;
}

constexpr const char* SNLUniverse::getTypeName() const {
  return "SNLUniverse";
}

std::string SNLUniverse::getString() const {
  return "SNLUniverse";
}

std::string SNLUniverse::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + getString() + ">";  
}

}
