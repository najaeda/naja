// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLInstTermOccurrence.h"

#include "SNLInstTerm.h"

namespace naja { namespace SNL {

SNLInstTermOccurrence::SNLInstTermOccurrence(SNLInstTerm* instTerm):
  super(instTerm)
{}

SNLInstTermOccurrence::SNLInstTermOccurrence(const SNLPath& path, SNLInstTerm* instTerm):
  super(path, instTerm)
{}

SNLInstTerm* SNLInstTermOccurrence::getInstTerm() const {
  return static_cast<SNLInstTerm*>(getObject());
}

}} // namespace SNL // namespace naja
