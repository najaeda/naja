// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLNet.h"

#include "PNLDesign.h"

namespace naja { namespace NL {

PNLNet::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

void PNLNet::preCreate() {
  super::preCreate();
}

void PNLNet::postCreate() {
  super::postCreate();
}

void PNLNet::preDestroy() {
  super::preDestroy();
}

//LCOV_EXCL_START
std::string PNLNet::Type::getString() const {
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

NLID::DesignObjectReference PNLNet::getReference() const {
  return NLID::DesignObjectReference(getDesign()->getReference(), getID());
}

}} // namespace NL // namespace naja