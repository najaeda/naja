// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLNet.h"

#include "SNLDesign.h"

namespace naja::NL {

void SNLNet::preCreate() {
  super::preCreate();
}

void SNLNet::postCreate() {
  super::postCreate();
}

void SNLNet::preDestroy() {
  super::preDestroy();
}

NLID::DesignObjectReference SNLNet::getReference() const {
  return NLID::DesignObjectReference(getDesign()->getReference(), getID());
}

}  // namespace naja::NL
