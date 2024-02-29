// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBitNet.h"

#include <list>

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLException.h"

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

void SNLBitNet::cloneComponents(SNLBitNet* newNet) const {
  newNet->components_.clone_from(
    components_,
    [newNet](const SNLNetComponent& component) -> SNLNetComponent* {
      auto newDesign = newNet->getDesign();
      if (auto scalarTerm = dynamic_cast<const SNLScalarTerm*>(&component)) {
        auto newScalarTerm = newDesign->getScalarTerm(scalarTerm->getID());
        if (newScalarTerm) {
          newScalarTerm->net_ = newNet;
          return newScalarTerm;
        } else {
          throw SNLException("SNLBitNet::cloneComponents: scalarTerm not found"); //LCOV_EXCL_LINE
        }
      } else if (auto busTermBit = dynamic_cast<const SNLBusTermBit*>(&component)) {
        auto busTerm = busTermBit->getBus();
        auto newBusTerm = newDesign->getBusTerm(busTerm->getID());
        if (newBusTerm) {
          auto newBusTermBit = newBusTerm->getBit(busTermBit->getBit());
          if (newBusTermBit) {
            newBusTermBit->net_ = newNet;
            return newBusTermBit;
          } else {
            throw SNLException("SNLBitNet::cloneComponents: busTermBit not found"); //LCOV_EXCL_LINE
          }
        } else {
          throw SNLException("SNLBitNet::cloneComponents: busTerm not found"); //LCOV_EXCL_LINE
        }
      } else if (auto instTerm = dynamic_cast<const SNLInstTerm*>(&component)) {
        auto instance = instTerm->getInstance();
        auto newInstance = newDesign->getInstance(instance->getID());
        if (newInstance) {
          auto newInstTerm = newInstance->getInstTerm(instTerm->getTerm());
          if (newInstTerm) {
            newInstTerm->net_ = newNet;
            return newInstTerm;
          } else {
            throw SNLException("SNLBitNet::cloneComponents: instTerm not found"); //LCOV_EXCL_LINE
          }
        } else {
          throw SNLException("SNLBitNet::cloneComponents: instance not found"); //LCOV_EXCL_LINE
        }
      } else {
        throw SNLException("SNLBitNet::cloneComponents: unknown component type"); //LCOV_EXCL_LINE
      }
    },
    [](SNLNetComponent*){} //LCOV_EXCL_LINE
  );
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