// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLEquipotential.h"

#include "SNLPath.h"
#include "SNLInstTermOccurrence.h"
#include "SNLBitTermOccurrence.h"
#include "SNLBitNetOccurrence.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"

namespace {

using namespace naja::SNL;

struct SNLEquipotentialExtractor {
  explicit SNLEquipotentialExtractor(
    SNLEquipotential::InstTermOccurrences& instTermOccurrences,
    SNLEquipotential::Terms& terms
  ): instTermOccurrences_(instTermOccurrences),
  terms_(terms)
  {}

  void extractNetComponentsFromNetOccurrence(
    const naja::SNL::SNLBitNetOccurrence& netOccurrence,
    const SNLNetComponentOccurrence& comingFromComponent,
    bool start
  ) {
    if (visitedNetOccurrences_.find(netOccurrence) != visitedNetOccurrences_.end()) {
      //already visited... report loop ??
      return;
    }
    visitedNetOccurrences_.insert(netOccurrence);
    auto net = netOccurrence.getNet();
    auto path = netOccurrence.getPath();
    for (auto component: net->getComponents()) {
      if (not start and component == comingFromComponent.getComponent()) {
        continue;
      }
      if (auto instTerm = dynamic_cast<SNLInstTerm*>(component)) {
        auto instance = instTerm->getInstance();
        if (instance->isLeaf()) {
          //construct InstTerm Occurrences
          instTermOccurrences_.emplace(naja::SNL::SNLInstTermOccurrence(path, instTerm));
        } else {
          //get inside instance by exploring from term occurrence
          auto term = instTerm->getBitTerm();
          auto instancePath = naja::SNL::SNLPath(path, instance);
          //
          extractNetFromNetComponentOccurrence(naja::SNL::SNLBitTermOccurrence(instancePath, term), false);
        }
      } else {
        auto term = static_cast<SNLBitTerm*>(component);
        if (path.empty()) {
          //top term: end exploration here
          terms_.insert(term);      
        } else {
          for (auto term: net->getBitTerms()) {
            //go up: extract from upper instance term occurrence
            auto parentPath = path.getHeadPath();
            auto instance = path.getTailInstance();
            auto instTerm = instance->getInstTerm(term);
            //
            extractNetFromNetComponentOccurrence(naja::SNL::SNLInstTermOccurrence(parentPath, instTerm), false);
          }
        }
      }
    }
  }
  
  //start extraction from net component
  //construct net and start exploration
  void extractNetFromNetComponentOccurrence(
    const naja::SNL::SNLNetComponentOccurrence& netComponentOccurrence,
    bool start
  ) {
    auto netOccurrence = netComponentOccurrence.getNetOccurrence();
    if (netOccurrence.isValid()) {
      extractNetComponentsFromNetOccurrence(netOccurrence, netComponentOccurrence, start);
    }
  } 

#if 0
    auto netOccurrence = netComponentOccurrence.getNetOccurrence();
    if (visitedNetOccurrences_.find(netOccurrence) != visitedNetOccurrences_.end()) {
      auto net = netOccurrence.getNet();
      auto path = netOccurrence.getPath();
      for (auto component: net->getComponents()) {
        auto instTerm = dynamic_cast<SNLInstTerm*>(component);
        if (instTerm) {

        } else {
          auto term = static_cast<SNLBitTerm*>(component);
          if (path.empty()) {
            
          } else {
            //construct term occurrence and go up
            auto termOccurrence = SNLBitTermOccurrence(path.getHeadPath(), term);
            extractFromNetComponentOccurrence(termOccurrence);
          }
        }
      }
    }
    naja::SNL::SNLPath path = netComponentOccurrence.getPath();
    naja::SNL::SNLBitNet* net = netComponentOccurrence.getNet();

    if (net) {
      //explore NetOccurrence
      naja::SNL::SNLBitNetOccurrence netOccurrence(path, net);
      extractFromNetOccurrence(netOccurrence);
    }
  }
#endif

  private:
    SNLEquipotential::InstTermOccurrences&  instTermOccurrences_;
    SNLEquipotential::Terms&                terms_;
    using NetOccurrences = std::set<SNLBitNetOccurrence>;
    NetOccurrences                          visitedNetOccurrences_  {};
};

}

namespace naja { namespace SNL {

SNLEquipotential::SNLEquipotential(SNLNetComponent* netComponent):
  SNLEquipotential(SNLNetComponentOccurrence(netComponent))
{}

SNLEquipotential::SNLEquipotential(const SNLNetComponentOccurrence& netComponentOccurrence) {
  SNLEquipotentialExtractor extractor(instTermOccurrences_, terms_);
  extractor.extractNetFromNetComponentOccurrence(netComponentOccurrence, true);
}

}} // namespace SNL // namespace naja
