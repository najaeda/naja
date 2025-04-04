// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBitTerm.h"

#include "NLException.h"

#include "SNLDesign.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLMacros.h"

namespace naja { namespace NL {

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

}} // namespace NL // namespace naja