// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLTerm.h"

#include "SNLDesign.h"

namespace naja::NL {

void SNLTerm::preCreate() {
  super::preCreate();
}

void SNLTerm::postCreate() {
  super::postCreate();
}

void SNLTerm::preDestroy() {
  super::preDestroy();
}

NLID::DesignObjectReference SNLTerm::getReference() const {
  return NLID::DesignObjectReference(getDesign()->getReference(), getID());
}

}  // namespace naja::NL