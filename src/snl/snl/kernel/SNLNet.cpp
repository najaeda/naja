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

#include "SNLNet.h"

#include "SNLDesign.h"

namespace naja { namespace SNL {

SNLNet::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

void SNLNet::preCreate() {
  super::preCreate();
}

void SNLNet::postCreate() {
  super::postCreate();
}

void SNLNet::preDestroy() {
  super::preDestroy();
}

//LCOV_EXCL_START
std::string SNLNet::Type::getString() const {
  switch (typeEnum_) {
    case Type::Standard: return "Standard";
    case Type::Assign0:  return "Assign0";
    case Type::Assign1:  return "Assign1";
    case Type::Supply0:  return "Supply0";
    case Type::Supply1:  return "Supply1";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

SNLID::DesignObjectReference SNLNet::getReference() const {
  return SNLID::DesignObjectReference(getDesign()->getReference(), getID());
}

}} // namespace SNL // namespace naja
