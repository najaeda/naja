// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBitNetOccurrence.h"

#include "SNLBitNet.h"

namespace naja { namespace SNL {

SNLBitNetOccurrence::SNLBitNetOccurrence(const SNLPath& path, SNLBitNet* net):
  SNLOccurrence(path, net)
{}

SNLBitNet* SNLBitNetOccurrence::getNet() const {
  return static_cast<SNLBitNet*>(getObject());
}

}} // namespace SNL // namespace naja
