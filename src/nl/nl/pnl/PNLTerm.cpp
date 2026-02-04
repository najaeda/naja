// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLTerm.h"

#include "PNLDesign.h"

namespace naja::NL {

void PNLTerm::preCreate() {
  super::preCreate();
}

void PNLTerm::postCreate() {
  super::postCreate();
}

void PNLTerm::preDestroy() {
  super::preDestroy();
}

NLID::DesignObjectReference PNLTerm::getReference() const {
  return NLID::DesignObjectReference(getDesign()->getReference(), getID());
}

}  // namespace naja::NL