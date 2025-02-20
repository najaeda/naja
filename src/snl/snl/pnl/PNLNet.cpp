// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLNet.h"

#include "PNLDesign.h"
#include "PNLTerm.h"
#include "PNLInstTerm.h"

namespace naja { namespace PNL {

PNLNet* PNLNet::create(PNLDesign* design, const naja::SNL::SNLName& name, naja::SNL::SNLID::DesignObjectID id) {
  auto net = new PNLNet();
  net->design_ = design;
  net->name_ = name;
  net->id_ = id;
  net->preCreate();
  net->postCreate();
  return net;
}

void PNLNet::addTerm(PNLTerm* term) {
  terms_.push_back(term);
}

PNLTerm* PNLNet::getTerm(naja::SNL::SNLID::DesignObjectID id) const {
  if (id >= terms_.size()) {
  }
    return terms_[id];
}

void PNLNet::detachTerm(naja::SNL::SNLID::DesignObjectID id) {
  if (id >= terms_.size()) {
  }
    terms_[id] = nullptr;
}

void PNLNet::addInstTerm(PNLInstTerm* instTerm) {
  instTerms_.push_back(instTerm);
}

PNLInstTerm* PNLNet::getInstTerm(naja::SNL::SNLID::DesignObjectID id) const {
  if (id >= instTerms_.size()) {
  }
    return instTerms_[id];
}

void PNLNet::detachInstTerm(naja::SNL::SNLID::DesignObjectID id) {
  if (id >= instTerms_.size()) {
  }
    instTerms_[id] = nullptr;
}

void PNLNet::preCreate() {
  super::preCreate();
}

void PNLNet::postCreate() {
  super::postCreate();
}

PNLDesign* PNLNet::getDesign() const {
  return design_;
}

SNL::SNLID::DesignObjectID PNLNet::getID() const {
  return id_;
}

void PNLNet::preDestroy() {
  design_->detachNet(id_);
  super::preDestroy();
}

void PNLNet::destroyFromDesign() {
  preDestroy();
  delete this;
}

}} // namespace PNL // namespace naja
