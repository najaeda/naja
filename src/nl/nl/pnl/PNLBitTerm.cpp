// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLBitTerm.h"

#include "NLException.h"

#include "PNLDesign.h"
#include "PNLBitNet.h"
#include "SNLMacros.h"

namespace naja::NL {

void PNLBitTerm::preCreate() {
  super::preCreate();
}

void PNLBitTerm::postCreate() {
  super::postCreate();
}

void PNLBitTerm::preDestroy() {
  super::preDestroy();
}

PNL_NET_COMPONENT_SET_NET(PNLBitTerm)

}  // namespace naja::NL
