// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLUniverse.h"

#include <iostream>
#include "NLDB0.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "NLException.h"

namespace naja { namespace NL {

NLUniverse* NLUniverse::universe_ = nullptr;

NLUniverse* NLUniverse::create() {
  preCreate();
  universe_ = new NLUniverse();
  universe_->postCreate();
  return universe_;
}

void NLUniverse::preCreate() {
  super::preCreate();
  if (universe_) {
    throw NLException("NLUniverse already exists");
  }
}

void NLUniverse::postCreate() {
  super::postCreate();
  //create the special DB0 which holds the NL managed libraries
  //such as assign cell
  db0_ = NLDB0::create(this);
}

void NLUniverse::preDestroy() {
#ifdef NL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  struct destroyDBFromUniverse {
    void operator()(NL::NLDB* db) {
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

NLUniverse* NLUniverse::get() {
  return universe_;
}

void NLUniverse::addDBAndSetID(NLDB* db) {
  if (dbs_.empty()) {
    db->id_ = 0;
  } else {
    auto it = dbs_.rbegin();
    NLDB* lastDB = &(*it);
    NLID::DBID dbID = lastDB->id_;
    ++dbID;
    db->id_ = dbID;
  }
  addDB(db);
}

void NLUniverse::addDB(NLDB* db) {
  //FIXME ?? are dbs sorted by id and if yes if we create a new one will
  //it be inserted at correct position ?
  dbs_.insert(*db);
}

void NLUniverse::removeDB(NLDB* db) {
  dbs_.erase(*db);
}

bool NLUniverse::isDB0(const NLDB* db) {
  auto universe = get();
  if (universe) {
    return db == universe->db0_;
  }
  return false;
}

NLDB* NLUniverse::getDB0() {
  auto universe = get();
  if (universe) {
    return universe->db0_;
  }
  return nullptr;
}

NLDB* NLUniverse::getDB(NLID::DBID id) const {
  auto it = dbs_.find(NLID(id), NLIDComp<NLDB>());
  if (it != dbs_.end()) {
    return const_cast<NLDB*>(&*it);
  }
  return nullptr;
}

NLLibrary* NLUniverse::getLibrary(NLID::DBID dbid, NLID::LibraryID libraryID) const {
  auto db = getDB(dbid);
  if (db) {
    return db->getLibrary(libraryID);
  }
  return nullptr;
}

SNLDesign* NLUniverse::getSNLDesign(const NLID::DesignReference& reference) const {
  auto db = getDB(reference.dbID_);
  if (db) {
    return db->getSNLDesign(reference.getDBDesignReference());
  }
  return nullptr;
}

SNLDesign* NLUniverse::getSNLDesign(const NLName& name) const {
  for (auto db: getDBs()) {
    auto design = db->getSNLDesign(name);
    if (design) {
      return design;
    }
  }
  return nullptr;
}

SNLTerm* NLUniverse::getTerm(const NLID::DesignObjectReference& reference) const {
  auto design = getSNLDesign(reference.getDesignReference());
  if (design) {
    return design->getTerm(reference.designObjectID_);
  }
  return nullptr;
}

SNLNet* NLUniverse::getNet(const NLID::DesignObjectReference& reference) const {
  auto design = getSNLDesign(reference.getDesignReference());
  if (design) {
    return design->getNet(reference.designObjectID_);
  }
  return nullptr;
}

SNLBitNet* NLUniverse::getBitNet(const NLID::BitNetReference& reference) const {
  auto design = getSNLDesign(reference.getDesignReference());
  if (design) {
    if (reference.isBusBit_) {
      return design->getBusNetBit(reference.designObjectID_, reference.bit_);
    } else {
      return design->getScalarNet(reference.designObjectID_);
    }
  }
  return nullptr;
}

SNLInstance* NLUniverse::getInstance(const NLID::DesignObjectReference& reference) const {
  auto design = getSNLDesign(reference.getDesignReference());
  if (design) {
    return design->getInstance(reference.designObjectID_);
  }
  return nullptr;
}

SNLInstTerm* NLUniverse::getInstTerm(const NLID& id) const {
  auto instance = getInstance(NLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.instanceID_));
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

SNLBusNetBit* NLUniverse::getBusNetBit(const NLID& id) const {
  SNLNet* net = getNet(NLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.designObjectID_));
  if (net) {
    if (auto busNet = dynamic_cast<SNLBusNet*>(net)) {
      return busNet->getBit(id.bit_);
    }
  }
  return nullptr;
}

SNLBusTermBit* NLUniverse::getBusTermBit(const NLID& id) const {
  SNLTerm* term = getTerm(NLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.designObjectID_));
  if (term) {
    if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
      return busTerm->getBit(id.bit_);
    }
  }
  return nullptr;
}

NLObject* NLUniverse::getObject(const NLID& id) {
  switch (id.type_) {
    case NLID::Type::DB:
      return getDB(id.dbID_);
    case NLID::Type::Library:
      return getLibrary(id.dbID_, id.libraryID_);
    case NLID::Type::Design:
      return getSNLDesign(NLID::DesignReference(id.dbID_, id.libraryID_, id.designID_));
    case NLID::Type::Instance:
      return getInstance(NLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.instanceID_));
    case NLID::Type::Term:
      return getTerm(NLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.designObjectID_));
    case NLID::Type::TermBit:
      return getBusTermBit(id);
    case NLID::Type::Net:
      return getNet(NLID::DesignObjectReference(id.dbID_, id.libraryID_, id.designID_, id.designObjectID_));
    case NLID::Type::NetBit:
      return getBusNetBit(id);
    case NLID::Type::InstTerm:
      return getInstTerm(id);
  }
  return nullptr; //LCOV_EXCL_LINE
}

NajaCollection<NLDB*> NLUniverse::getDBs() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&dbs_));
}

NajaCollection<NLDB*> NLUniverse::getUserDBs() const {
  auto filter = [](const NLDB* db) {return not NLUniverse::isDB0(db); };
  return getDBs().getSubCollection(filter);
}

NLDB* NLUniverse::getTopDB() {
  auto universe = get();
  if (universe) {
    return universe->topDB_;
  }
  return nullptr;
}

void NLUniverse::setTopDB(NLDB* db) {
  if (NLDB0::isDB0(db)) {
    throw NLException("Cannot set DB0 as top DB");
  }
  topDB_ = db;
}

void NLUniverse::setTopDesign(SNLDesign* design) {
  auto db = design->getDB();
  db->setTopDesign(design);
  setTopDB(db);
}

SNLDesign* NLUniverse::getTopDesign() {
  auto topDB = getTopDB();
  if (topDB) {
    return topDB->getTopDesign();
  }
  return nullptr;
}

void NLUniverse::mergeAssigns() {
  for (auto db: getUserDBs()) {
    db->mergeAssigns();
  }
}

//LCOV_EXCL_START
const char* NLUniverse::getTypeName() const {
  return "NLUniverse";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string NLUniverse::getString() const {
  return "NLUniverse";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string NLUniverse::getDescription() const {
  return "<" + std::string(getTypeName()) + ">";  
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void NLUniverse::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    for (auto db: getDBs()) {
      db->debugDump(indent+2, recursive, stream);
    }
  }
}
//LCOV_EXCL_STOP

}} // namespace NL // namespace naja
