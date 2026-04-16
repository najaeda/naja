// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLDesignObject.h"
#include "NLDB.h"
#include "NLLibrary.h"

namespace naja::NL {

naja::NL::NLID PNLDesignObject::getNLID(const naja::NL::NLID::Type& type,
                                naja::NL::NLID::DesignObjectID objectID,
                                naja::NL::NLID::DesignObjectID instanceID,
                                naja::NL::NLID::Bit bit) const {
  return naja::NL::NLID(type, getDB()->getID(), getLibrary()->getID(),
               getDesign()->getID(), objectID, instanceID, bit);
}

naja::NL::NLLibrary* PNLDesignObject::getLibrary() const {
  return getDesign()->getLibrary();
}

naja::NL::NLDB* PNLDesignObject::getDB() const {
  return getLibrary()->getDB();
}

void PNLDesignObject::preCreate() {}

void PNLDesignObject::postCreate() {}

}  // namespace naja::NL
