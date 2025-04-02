// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLBitNet.h"

#include <list>

#include "NLDB.h"
#include "NLException.h"

//#include "PNLScalarTerm.h"
//#include "PNLBusTerm.h"
//#include "PNLBusTermBit.h"
#include "PNLInstTerm.h"
#include "PNLDesign.h"
#include "PNLBitTerm.h"
#include "PNLScalarTerm.h"

namespace naja { namespace NL {

void PNLBitNet::preCreate() {
  super::preCreate();
}

void PNLBitNet::postCreate() {
  super::postCreate();
}

void PNLBitNet::preDestroy() {
  using Components = std::list<PNLNetComponent*>;
  Components components(getComponents().begin(), getComponents().end());
  for (auto component: components) {
    component->setNet(nullptr);
  }
  super::preDestroy();
}

// void PNLBitNet::cloneComponents(PNLBitNet* newNet) const {
//   newNet->components_.clone_from(
//     components_,
//     [newNet](const PNLNetComponent& component) -> PNLNetComponent* {
//       auto newDesign = newNet->getDesign();
//       if (auto scalarTerm = dynamic_cast<const PNLScalarTerm*>(&component)) {
//         auto newScalarTerm = newDesign->getScalarTerm(scalarTerm->getID());
//         if (newScalarTerm) {
//           newScalarTerm->net_ = newNet;
//           return newScalarTerm;
//         } else {
//           throw NLException("PNLBitNet::cloneComponents: scalarTerm not found"); //LCOV_EXCL_LINE
//         }
//       // } else if (auto busTermBit = dynamic_cast<const PNLBusTermBit*>(&component)) {
//       //   auto busTerm = busTermBit->getBus();
//       //   auto newBusTerm = newDesign->getBusTerm(busTerm->getID());
//       //   if (newBusTerm) {
//       //     auto newBusTermBit = newBusTerm->getBit(busTermBit->getBit());
//       //     if (newBusTermBit) {
//       //       newBusTermBit->net_ = newNet;
//       //       return newBusTermBit;
//       //     } else {
//       //       throw NLException("PNLBitNet::cloneComponents: busTermBit not found"); //LCOV_EXCL_LINE
//       //     }
//       //   } else {
//       //     throw NLException("PNLBitNet::cloneComponents: busTerm not found"); //LCOV_EXCL_LINE
//       //   }
//       } else if (auto instTerm = dynamic_cast<const PNLInstTerm*>(&component)) {
//         auto instance = instTerm->getInstance();
//         auto newInstance = newDesign->getInstance(instance->getID());
//         if (newInstance) {
//           auto newInstTerm = newInstance->getInstTerm(instTerm->getBitTerm());
//           if (newInstTerm) {
//             newInstTerm->net_ = newNet;
//             return newInstTerm;
//           } else {
//             throw NLException("PNLBitNet::cloneComponents: instTerm not found"); //LCOV_EXCL_LINE
//           }
//         } else {
//           throw NLException("PNLBitNet::cloneComponents: instance not found"); //LCOV_EXCL_LINE
//         }
//       } else {
//         throw NLException("PNLBitNet::cloneComponents: unknown component type"); //LCOV_EXCL_LINE
//       }
//     },
//     [](PNLNetComponent*){} //LCOV_EXCL_LINE
//   );
// }

NajaCollection<PNLNetComponent*> PNLBitNet::getComponents() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&components_));
}

NajaCollection<PNLInstTerm*> PNLBitNet::getInstTerms() const {
  return getComponents().getSubCollection<PNLInstTerm*>();
}

NajaCollection<PNLBitTerm*> PNLBitNet::getBitTerms() const {
  return getComponents().getSubCollection<PNLBitTerm*>();
}

void PNLBitNet::addComponent(PNLNetComponent* component) {
  //FIXME: should assert that component is not in bitNet ? not connected hook ?
  components_.insert(*component);
}

void PNLBitNet::removeComponent(PNLNetComponent* component) {
  components_.erase(*component);
}

// void PNLBitNet::connectAllComponentsTo(PNLBitNet* net) {
//   if (net not_eq this) {
//     using Components = std::list<PNLNetComponent*>;
//     Components components(getComponents().begin(), getComponents().end());
//     for (auto component: components) {
//       component->setNet(net);
//     }
//   }
//   components_.clear();
// }

}} // namespace NL // namespace naja