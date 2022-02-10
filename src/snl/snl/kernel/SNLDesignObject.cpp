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

#include "SNLDesignObject.h"

#include "Card.h"

#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"

namespace naja { namespace SNL {

void SNLDesignObject::postCreate() {
  super::postCreate();
}

void SNLDesignObject::preDestroy() {
  super::preDestroy();
}

SNLLibrary* SNLDesignObject::getLibrary() const {
  return getDesign()->getLibrary();
}

SNLDB* SNLDesignObject::getDB() const {
  return getLibrary()->getDB();
}

SNLID SNLDesignObject::getSNLID(
    const SNLID::Type& type,
    SNLID::DesignObjectID objectID,
    SNLID::DesignObjectID instanceID,
    SNLID::Bit bit) const {
  return SNLID(type,
      getDB()->getID(),
      getLibrary()->getID(),
      getDesign()->getID(),
      objectID,
      instanceID,
      bit);
}

}} // namespace SNL // namespace naja