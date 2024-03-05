// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBitTerm.h"
#include "SNLDesign.h"
#include "SNLBitNet.h"
#include "SNLException.h"
#include "SNLMacros.h"

namespace naja { namespace SNL {

void SNLBitTerm::preCreate() {
  super::preCreate();
}

void SNLBitTerm::postCreate() {
  super::postCreate();
}

void SNLBitTerm::preDestroy() {
  super::preDestroy();
}

NET_COMPONENT_SET_NET(SNLBitTerm)

}} // namespace SNL // namespace naja
