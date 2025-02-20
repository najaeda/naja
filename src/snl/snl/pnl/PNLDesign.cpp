// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLDesign.h"

#include "SNLDB.h"
#include "SNLLibrary.h"

using namespace naja::SNL; 

namespace naja { namespace PNL {

PNLDesign::PNLDesign(SNLLibrary* library):
  super(), library_(library), origin_(0, 0)
{}

PNLDesign* PNLDesign::create(SNLLibrary* library) {
  preCreate(library);
  auto design = new PNLDesign(library);
  design->postCreateAndSetID();
  return design;
}

void PNLDesign::preCreate(const SNLLibrary* library) {
  super::preCreate();
}

void PNLDesign::postCreateAndSetID() {
  super::postCreate();
  library_->addPNLDesignAndSetID(this);
}

void PNLDesign::postCreate() {
  super::postCreate();
  library_->addPNLDesign(this);
}

void PNLDesign::preDestroy() {
  for (auto term : terms_) {
    if (term) {
      term->destroyFromDesign();
    }
  }
  super::preDestroy();
}

//LCOV_EXCL_START
const char* PNLDesign::getTypeName() const {
  return "PNLDesign";
}
//LCOV_EXCL_STOP

std::string PNLDesign::getString() const {
  return "PNLDesign";
}

std::string PNLDesign::getDescription() const {
  return "PNLDesign";
}

bool PNLDesign::deepCompare(const PNLDesign* other, std::string& reason) const {
  return false;
}

void PNLDesign::debugDump(size_t indent, bool recursive, std::ostream& stream) const {

}

SNLDB* PNLDesign::getDB() const {
  return library_->getDB();
}

SNLID PNLDesign::getSNLID() const {
  return SNLID(getDB()->getID(), library_->getID(), getID());
}

void PNLDesign::addTerm(PNLTerm* term) {
  terms_.push_back(term);
}

PNLTerm* PNLDesign::getTerm(SNLID::DesignObjectID id) const {
  if (id >= terms_.size()) {
    // TODO: throw exception
    return nullptr;
  }
  return terms_[id];
}

void PNLDesign::detachTerm(SNLID::DesignObjectID id) {
  if (id >= terms_.size()) {
    // TODO: throw exception
    return;
  }
  terms_[id] = nullptr;
}

void PNLDesign::detachNet(SNLID::DesignObjectID id) {
  if (id >= nets_.size()) { 
    // TODO: throw exception
  }
  nets_[id] = nullptr;
}

void PNLDesign::addNet(PNLNet* net) {
  nets_.push_back(net);
}

PNLNet* PNLDesign::getNet(SNLID::DesignObjectID id) const {
  if (id >= nets_.size()) {
    // TODO: throw exception
    return nullptr;
  }
  return nets_[id];
}

void PNLDesign::detachInstance(SNLID::DesignObjectID id) {
  if (id >= instances_.size()) {
    // TODO: throw exception
  }
  instances_[id] = nullptr;
}

void PNLDesign::addInstance(PNLInstance* instance) {
  instances_.push_back(instance);
}

PNLInstance* PNLDesign::getInstance(SNLID::DesignObjectID id) const {
  if (id >= instances_.size()) {
    // TODO: throw exception
  }
  return instances_[id];
}

}} // namespace PNL // namespace naja