// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBitTermOccurrence.h"

#include "SNLBitTerm.h"

namespace naja { namespace SNL {

SNLBitTermOccurrence::SNLBitTermOccurrence(SNLBitTerm* term):
  super(term)
{}

SNLBitTermOccurrence::SNLBitTermOccurrence(const SNLPath& path, SNLBitTerm* term):
  super(path, term)
{}

SNLBitTerm* SNLBitTermOccurrence::getTerm() const {
  return static_cast<SNLBitTerm*>(getObject());
}

}} // namespace SNL // namespace naja
