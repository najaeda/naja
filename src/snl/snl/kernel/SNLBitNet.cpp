// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBitNet.h"

#include <list>

#include "SNLBitTerm.h"
#include "SNLInstTerm.h"

namespace naja { namespace SNL {

void SNLBitNet::preCreate() {
  super::preCreate();
}

void SNLBitNet::postCreate() {
  super::postCreate();
}

void SNLBitNet::preDestroy() {
  using Components = std::list<SNLNetComponent*>;
  Components components(getComponents().begin(), getComponents().end());
  for (auto component: components) {
    component->setNet(nullptr);
  }
  super::preDestroy();
}

NajaCollection<SNLNetComponent*> SNLBitNet::getComponents() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&components_));
}

NajaCollection<SNLInstTerm*> SNLBitNet::getInstTerms() const {
  return getComponents().getSubCollection<SNLInstTerm*>();
}

NajaCollection<SNLBitTerm*> SNLBitNet::getBitTerms() const {
  return getComponents().getSubCollection<SNLBitTerm*>();
}

void SNLBitNet::addComponent(SNLNetComponent* component) {
  //FIXME: should assert that component is not in bitNet ? not connected hook ?
  components_.insert(*component);
}

void SNLBitNet::removeComponent(SNLNetComponent* component) {
  components_.erase(*component);
}

void SNLBitNet::connectAllComponentsTo(SNLBitNet* net) {
  if (net not_eq this) {
    using Components = std::list<SNLNetComponent*>;
    Components components(getComponents().begin(), getComponents().end());
    for (auto component: components) {
      component->setNet(net);
    }
  }
  components_.clear();
}

}} // namespace SNL // namespace naja