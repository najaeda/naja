// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignObject.h"

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