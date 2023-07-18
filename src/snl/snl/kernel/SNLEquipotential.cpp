/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

  void extractNetComponentFromNetOccurrence(
    const naja::SNL::SNLBitNetOccurrence& netOccurrence
  ) {
    if (visitedNetOccurrences_.find(netOccurrence) != visitedNetOccurrences_.end()) {
      //already visited... report loop ??
      return;
    }
    visitedNetOccurrences_.insert(netOccurrence);
    auto net = netOccurrence.getNet();
    auto path = netOccurrence.getPath();
    for (auto component: net->getComponents()) {
      if (auto instTerm = dynamic_cast<SNLInstTerm*>(component)) {
        auto instance = instTerm->getInstance();
        if (instance->isLeaf()) {
          //construct InstTerm Occurrences
          instTermOccurrences_.emplace(naja::SNL::SNLInstTermOccurrence(path, instTerm));
        } else {
          //get inside instance by exploring from term occurrence
          auto term = instTerm->getTerm();
          auto instancePath = naja::SNL::SNLPath(path, instance);
          //
          extractNetFromNetComponentOccurrence(naja::SNL::SNLBitTermOccurrence(instancePath, term));
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
            extractNetFromNetComponentOccurrence(naja::SNL::SNLInstTermOccurrence(parentPath, instTerm));
          }
        }
      }
    }
  }
  
  //start extraction from net component
  //construct net and start exploration
  void extractNetFromNetComponentOccurrence(
    const naja::SNL::SNLNetComponentOccurrence& netComponentOccurrence
  ) {
    auto netOccurrence = netComponentOccurrence.getNetOccurrence();
    if (netOccurrence.isValid()) {
      extractNetComponentFromNetOccurrence(netOccurrence);
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
    using NetOccurrences = std::set<const SNLBitNetOccurrence>;
    NetOccurrences                          visitedNetOccurrences_  {};
};

}

namespace naja { namespace SNL {

SNLEquipotential::SNLEquipotential(SNLNetComponent* netComponent):
  SNLEquipotential(SNLNetComponentOccurrence(netComponent))
{}

SNLEquipotential::SNLEquipotential(const SNLNetComponentOccurrence& netComponentOccurrence) {
  SNLEquipotentialExtractor extractor(instTermOccurrences_, terms_);
  extractor.extractNetFromNetComponentOccurrence(netComponentOccurrence);
}

}} // namespace SNL // namespace naja
