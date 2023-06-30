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
#include "SNLBitNetOccurrence.h"
#include "SNLNetComponentOccurrence.h"

namespace {

void extractFromNetComponentOccurrence(
  const naja::SNL::SNLNetComponentOccurrence& netComponentOccurrence
) {
  naja::SNL::SNLPath path = netComponentOccurrence.getPath();
  naja::SNL::SNLBitNet* net = netComponentOccurrence.getNet();

  if (net) {
    //explore NetOccurrence
    naja::SNL::SNLBitNetOccurrence netOccurrence(path, net);
  }
}

}

namespace naja { namespace SNL {

SNLEquipotential::SNLEquipotential(SNLNetComponent* netComponent):
  SNLEquipotential(SNLNetComponentOccurrence(netComponent))
{}

SNLEquipotential::SNLEquipotential(const SNLNetComponentOccurrence& netComponentOccurrence) {
  extractFromNetComponentOccurrence(netComponentOccurrence);
}

}} // namespace SNL // namespace naja
