// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLTerm.h"

#include "SNLDesign.h"

namespace naja { namespace SNL {

void SNLTerm::preCreate() {
  super::preCreate();
}

void SNLTerm::postCreate() {
  super::postCreate();
}

void SNLTerm::preDestroy() {
  super::preDestroy();
}

SNLID::DesignObjectReference SNLTerm::getReference() const {
  return SNLID::DesignObjectReference(getDesign()->getReference(), getID());
}

}} // namespace SNL // namespace naja