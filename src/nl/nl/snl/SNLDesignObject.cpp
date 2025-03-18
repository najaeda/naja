// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignObject.h"

#include "NLDB.h"
#include "NLLibrary.h"
#include "SNLDesign.h"

namespace naja { namespace SNL {

void SNLDesignObject::postCreate() {
  super::postCreate();
}

void SNLDesignObject::preDestroy() {
  super::preDestroy();
}

NLLibrary* SNLDesignObject::getLibrary() const {
  return getDesign()->getLibrary();
}

NLDB* SNLDesignObject::getDB() const {
  return getLibrary()->getDB();
}

NLID SNLDesignObject::getNLID(
    const NLID::Type& type,
    NLID::DesignObjectID objectID,
    NLID::DesignObjectID instanceID,
    NLID::Bit bit) const {
  return NLID(type,
      getDB()->getID(),
      getLibrary()->getID(),
      getDesign()->getID(),
      objectID,
      instanceID,
      bit);
}

NajaCollection<SNLAttribute> SNLDesignObject::getAttributes() const {
  return SNLAttributes::getAttributes(this);
}

}} // namespace SNL // namespace naja
