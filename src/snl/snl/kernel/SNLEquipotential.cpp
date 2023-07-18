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
#include "SNLInstTerm.h"
#include "SNLInstance.h"

namespace {

using namespace naja::SNL;

struct SNLEquipotentialExtractor {
  explicit SNLEquipotentialExtractor(
    SNLEquipotential::InstTermOccurrences& instTermOccurrences
  ): instTermOccurrences_(instTermOccurrences)
  {}

  void extractFromNetOccurrence(
    const naja::SNL::SNLBitNetOccurrence& netOccurrence
  ) {
    auto net = netOccurrence.getNet();
    auto path = netOccurrence.getPath();
    for (auto instTerm: net->getInstTerms()) {
      auto instance = instTerm->getInstance();
      if (instance->isLeaf()) {
        //construct InstTerm Occurrences
        instTermOccurrences_.emplace(naja::SNL::SNLInstTermOccurrence(path, instTerm));
      } else {
        //get inside instance by exploring from term occurrence
        auto term = instTerm->getTerm();
        auto instancePath = naja::SNL::SNLPath(path, instance);
        //
        extractFromNetComponentOccurrence(naja::SNL::SNLBitTermOccurrence(instancePath, term));
      }
    }
    if (path.empty()) {
    } else {
      for (auto term: net->getBitTerms()) {
        //go up: extract from upper instance term occurrence
        auto parentPath = path.getHeadPath();
        auto instance = path.getTailInstance();
        auto instTerm = instance->getInstTerm(term);
        //
        extractFromNetComponentOccurrence(naja::SNL::SNLInstTermOccurrence(parentPath, instTerm));
      }
    }
  }

  void extractFromNetComponentOccurrence(
    const naja::SNL::SNLNetComponentOccurrence& netComponentOccurrence
  ) {
    naja::SNL::SNLPath path = netComponentOccurrence.getPath();
    naja::SNL::SNLBitNet* net = netComponentOccurrence.getNet();

    if (net) {
      //explore NetOccurrence
      naja::SNL::SNLBitNetOccurrence netOccurrence(path, net);
      extractFromNetOccurrence(netOccurrence);
    }
  }

  private:
    SNLEquipotential::InstTermOccurrences&  instTermOccurrences_;

};

}

namespace naja { namespace SNL {

SNLEquipotential::SNLEquipotential(SNLNetComponent* netComponent):
  SNLEquipotential(SNLNetComponentOccurrence(netComponent))
{}

SNLEquipotential::SNLEquipotential(const SNLNetComponentOccurrence& netComponentOccurrence) {
  SNLEquipotentialExtractor extractor(instTermOccurrences_);
  extractor.extractFromNetComponentOccurrence(netComponentOccurrence);
}

}} // namespace SNL // namespace naja
