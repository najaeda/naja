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
    case Undefined: return "Undefined";
    case Logical:   return "Logical";
    case Clock:     return "Clock";
    case VDD:       return "VDD";
    case GND:       return "GND";
    case Blockage:  return "Blockage";
    case Analog:    return "Analog";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

NLID::DesignObjectReference PNLNet::getReference() const {
  return NLID::DesignObjectReference(getDesign()->getReference(), getID());
}

}} // namespace NL // namespace naja