// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLInstTerm.h"

#include "PNLDesign.h"
#include "PNLNet.h"
#include "PNLInstance.h"

namespace naja { namespace PNL {

PNLInstTerm* PNLInstTerm::create(PNLInstance* instance, PNLTerm* term, PNLNet* net,
                                   PNLTerm::Direction direction,
                                   naja::SNL::SNLID::DesignObjectID id) {
  auto instTerm = new PNLInstTerm();
  instTerm->instance_ = instance;
  instTerm->term_ = term;
  instTerm->net_ = net;
  instTerm->direction_ = direction;
  instTerm->id_ = id;
  instTerm->preCreate();
  instTerm->postCreate();
  return instTerm;
}

PNLTerm::Direction PNLInstTerm::getDirection() {
  return term_->getDirection();
}

PNLNet* PNLInstTerm::getNet() {
  return net_;
}

void PNLInstTerm::setNet(PNLNet* net) {
  net_ = net;
}

PNLInstance* PNLInstTerm::getInstance() const {
  return instance_;
}

naja::SNL::SNLID::DesignObjectID PNLInstTerm::getID() const {
  return id_;
}

void PNLInstTerm::preCreate() {
    super::preCreate();
}

void PNLInstTerm::postCreate() {
    super::postCreate();
}

void PNLInstTerm::preDestroy() {
  super::preDestroy();
}

void PNLInstTerm::destroyFromInstance() {
  preDestroy();
  delete this;
}

}} // namespace PNL // namespace naja


