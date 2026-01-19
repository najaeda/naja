// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLName.h"

#include "NLException.h"
#include "NLUniverse.h"

namespace naja { namespace NL {

NLName::NLName(const std::string& name) {
  auto universe = NLUniverse::get();
  if (not universe) {
    throw NLException("NLName creation requires NLUniverse");
  }
  if (name.empty()) {
    id_ = 0;
    return;
  }
  id_ = universe->getOrCreateNameID(name);
}

const std::string& NLName::getStringRef() const {
  if (auto universe = NLUniverse::get()) {
    return universe->getNameString(id_);
  }
  static const std::string empty;
  return empty;
}

}} // namespace NL // namespace naja
