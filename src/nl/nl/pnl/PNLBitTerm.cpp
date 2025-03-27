// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLBitTerm.h"

#include "NLException.h"

#include "PNLDesign.h"
#include "PNLBitNet.h"

namespace naja {
namespace NL {

void PNLBitTerm::preCreate() {
  super::preCreate();
}

void PNLBitTerm::postCreate() {
  super::postCreate();
}

void PNLBitTerm::preDestroy() {
  super::preDestroy();
}

void PNLBitTerm::setNet(PNLNet* net) {
  PNLBitNet* bitnet = dynamic_cast<PNLBitNet*>(net);
  assert(bitnet);
  if (net_) {
    net_->removeComponent(this);
  }
  net_ = bitnet;
  if (net_) {
    net_->addComponent(this);
  }
}
}  // namespace NL
}  // namespace naja