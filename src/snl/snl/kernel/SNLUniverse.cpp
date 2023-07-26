// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLUniverse.h"

#include <iostream>
#include "SNLDB0.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLException.h"

namespace naja { namespace SNL {

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
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  struct destroyDBFromUniverse {
    void operator()(SNL::SNLDB* db) {
      db->destroyFromUniverse();
    }
  };
  //Make sure that the last destroyed DB is DB0 
  if (dbs_.size()>1) {
    dbs_.erase_and_dispose(++dbs_.begin(), dbs_.end(), destroyDBFromUniverse());
  }
  dbs_.clear_and_dispose(destroyDBFromUniverse());
  universe_ = nullptr;
  super::preDestroy();
}

SNLUniverse* SNLUniverse::get() {
  return universe_;
}

void SNLUniverse::addDBAndSetID(SNLDB* db) {
  if (dbs_.empty()) {
    db->id_ = 0;
  } else {
    auto it = dbs_.rbegin();
    SNLDB* lastDB = &(*it);
    SNLID::DBID dbID = lastDB->id_;
    ++dbID;
    db->id_ = dbID;
  }
  addDB(db);
}

void SNLUniverse::addDB(SNLDB* db) {
  //FIXME ?? are dbs sorted by id and if yes if we create a new one will
  //it be inserted at correct position ?
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

SNLDB* SNLUniverse::getDB0() {
  auto universe = get();
  if (universe) {
    return universe->db0_;
  }
  return nullptr;
}

SNLDB* SNLUniverse::getDB(SNLID::DBID id) const {
  auto it = dbs_.find(SNLID(id), SNLIDComp<SNLDB>());
  if (it != dbs_.end()) {
    return const_cast<SNLDB*>(&*it);
  }
  return nullptr;
}

SNLLibrary* SNLUniverse::getLibrary(SNLID::DBID dbid, SNLID::LibraryID libraryID) const {
  auto db = getDB(dbid);
  if (db) {
    return db->getLibrary(libraryID);
  }
  return nullptr;
}

SNLDesign* SNLUniverse::getDesign(const SNLID::DesignReference& reference) const {
  auto db = getDB(reference.dbID_);
  if (db) {
    return db->getDesign(reference.getDBDesignReference());
  }
  return nullptr;
}

SNLDesign* SNLUniverse::getDesign(const SNLName& name) const {
  for (auto db: getDBs()) {
    auto design = db->getDesign(name);
    if (design) {
      return design;
    }
  }
  return nullptr;
}

SNLTerm* SNLUniverse::getTerm(const SNLID::DesignObjectReference& reference) const {
  auto design = getDesign(reference.getDesignReference());
  if (design) {
    return design->getTerm(reference.designObjectID_);
  }
  return nullptr;
}

SNLNet* SNLUniverse::getNet(const SNLID::DesignObjectReference& reference) const {
  auto design = getDesign(reference.getDesignReference());
  if (design) {
    return design->getNet(reference.designObjectID_);
  }
  return nullptr;
}

SNLBitNet* SNLUniverse::getBitNet(const SNLID::BitNetReference& reference) const {
  auto design = getDesign(reference.getDesignReference());
  if (design) {
    if (reference.isBusBit_) {
      return design->getBusNetBit(reference.designObjectID_, reference.bit_);
    } else {
      return design->getScalarNet(reference.designObjectID_);
    }
  }
  return nullptr;
}

SNLInstance* SNLUniverse::getInstance(const SNLID::DesignObjectReference& reference) const {
  auto design = getDesign(reference.getDesignReference());
  if (design) {
    return design->getInstance(reference.designObjectID_);
  }
  return nullptr;
}

SNLInstTerm* SNLUniverse::getInstTerm(const SNLID& id) const {
  auto instance = getInstance(SNLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.instanceID_));
  if (instance) {
    auto model = instance->getModel();
    if (model) {
      auto term = model->getTerm(id.designObjectID_);
      if (term) {
        if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
          return instance->getInstTerm(scalarTerm);
        } else {
          auto busTerm = static_cast<SNLBusTerm*>(term);
          auto busTermBit = busTerm->getBit(id.bit_);
          if (busTermBit) {
            return instance->getInstTerm(busTermBit);
          }
        }
      }
    }
  }
  return nullptr;
}

SNLBusNetBit* SNLUniverse::getBusNetBit(const SNLID& id) const {
  SNLNet* net = getNet(SNLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.designObjectID_));
  if (net) {
    if (auto busNet = dynamic_cast<SNLBusNet*>(net)) {
      return busNet->getBit(id.bit_);
    }
  }
  return nullptr;
}

SNLBusTermBit* SNLUniverse::getBusTermBit(const SNLID& id) const {
  SNLTerm* term = getTerm(SNLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.designObjectID_));
  if (term) {
    if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
      return busTerm->getBit(id.bit_);
    }
  }
  return nullptr;
}

SNLObject* SNLUniverse::getObject(const SNLID& id) {
  switch (id.type_) {
    case SNLID::Type::DB:
      return getDB(id.dbID_);
    case SNLID::Type::Library:
      return getLibrary(id.dbID_, id.libraryID_);
    case SNLID::Type::Design:
      return getDesign(SNLID::DesignReference(id.dbID_, id.libraryID_, id.designID_));
    case SNLID::Type::Instance:
      return getInstance(SNLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.instanceID_));
    case SNLID::Type::Term:
      return getTerm(SNLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.designObjectID_));
    case SNLID::Type::TermBit:
      return getBusTermBit(id);
    case SNLID::Type::Net:
      return getNet(SNLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.designObjectID_));
    case SNLID::Type::NetBit:
      return getBusNetBit(id);
    case SNLID::Type::InstTerm:
      return getInstTerm(id);
  }
  return nullptr; //LCOV_EXCL_LINE
}

NajaCollection<SNLDB*> SNLUniverse::getDBs() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&dbs_));
}

NajaCollection<SNLDB*> SNLUniverse::getUserDBs() const {
  auto filter = [](const SNLDB* db) {return not SNLUniverse::isDB0(db); };
  return getDBs().getSubCollection(filter);
}

SNLDB* SNLUniverse::getTopDB() const {
  return topDB_;
}

void SNLUniverse::setTopDB(SNLDB* db) {
  if (SNLDB0::isDB0(db)) {
    throw SNLException("Cannot set DB0 as top DB");
  }
  topDB_ = db;
}

void SNLUniverse::setTopDesign(SNLDesign* design) {
  auto db = design->getDB();
  db->setTopDesign(design);
  setTopDB(db);
}

SNLDesign* SNLUniverse::getTopDesign() const {
  if (topDB_) {
    return topDB_->getTopDesign();
  }
  return nullptr;
}

void SNLUniverse::mergeAssigns() {
  for (auto db: getUserDBs()) {
    db->mergeAssigns();
  }
}

//LCOV_EXCL_START
const char* SNLUniverse::getTypeName() const {
  return "SNLUniverse";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLUniverse::getString() const {
  return "SNLUniverse";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLUniverse::getDescription() const {
  return "<" + std::string(getTypeName()) + ">";  
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLUniverse::debugDump(size_t indent, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  for (auto db: getDBs()) {
    db->debugDump(indent+2, stream);
  }
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
