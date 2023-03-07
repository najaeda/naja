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

}} // namespace SNL // namespace naja