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

#include "SNLNetComponentOccurrence.h"

#include "SNLNetComponent.h"
#include "SNLPath.h"
#include "SNLBitNetOccurrence.h"

namespace naja { namespace SNL {

SNLNetComponentOccurrence::SNLNetComponentOccurrence(SNLNetComponent* component):
  super(component)
{}

SNLNetComponentOccurrence::SNLNetComponentOccurrence(const SNLPath& path, SNLNetComponent* component):
  super(path, component)
{}

SNLNetComponent* SNLNetComponentOccurrence::getComponent() const {
  return static_cast<SNLNetComponent*>(getObject());
}

SNLBitNetOccurrence SNLNetComponentOccurrence::getNetOccurrence() const {
  auto net = getNet();
  if (net) {
    return SNLBitNetOccurrence(getPath(), net);
  }
  return SNLBitNetOccurrence();
}

SNLBitNet* SNLNetComponentOccurrence::getNet() const {
  auto component = getComponent();
  if (component) {
    return component->getNet();
  }
  return nullptr;
}

}} // namespace SNL // namespace naja
