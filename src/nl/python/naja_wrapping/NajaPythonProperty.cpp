// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaPythonProperty.h"

#include <sstream>
#include <cstddef>
#include <cstdint>

#include "NajaProperty.h"
#include "NajaObject.h"
#include "NLException.h"

namespace naja {

// ---------------------------------------------------------------
// Local Variables.

const char* twiceSetOffset =
    "NajaPythonProperty::setOffset () :\n\n"
    "    Attempt to sets the _shadow member offset twice.\n";

// -------------------------------------------------------------------
// Class : "SNLProxyProperty".

std::string NajaPythonProperty::name_ = "Naja::SNLProxyProperty";
std::ptrdiff_t NajaPythonProperty::offset_ = -1;

NajaPythonProperty::NajaPythonProperty(void* shadow):
  shadow_(shadow)
{}

NajaPythonProperty* NajaPythonProperty::create(void* shadow) {
  if (not shadow) {
    throw NL::NLException(
        "SNLProxyProperty::create(): Empty \"shadow\" argument.");
  }

  NajaPythonProperty* property = new NajaPythonProperty(shadow);
  //property->postCreate(); FIXME

  return property;
}

void NajaPythonProperty::preDestroy() {
  
  if (owner_) {
    owner_->onDestroyed(this);
  }
  if (offset_ >= 0) {
    // Portable pointer arithmetic: Windows is LLP64 (unsigned long is 32-bit),
    // so never cast pointers to unsigned long.
    auto* base = reinterpret_cast<std::byte*>(shadow_);
    auto* addr = base + static_cast<std::ptrdiff_t>(offset_);

    // Defensive: ensure we are writing to an address aligned for a pointer.
    if ((reinterpret_cast<std::uintptr_t>(addr) % alignof(void*)) != 0) {
      // If this triggers, the offset computation is wrong or the struct packing differs.
      // LCOV_EXCL_START
      throw NL::NLException(
        "NajaPythonProperty::preDestroy(): shadow member offset is misaligned (likely non-portable offset / packing issue)."
      );
      // LCOV_EXCL_STOP
    }

    auto** shadowMember = reinterpret_cast<void**>(addr);
    *shadowMember = nullptr;
  }
}

void NajaPythonProperty::onCapturedBy(NajaObject* owner) {
  if (owner_ and (owner_ != owner)) {
    throw NL::NLException(getString().c_str());
  }

  owner_ = owner;
}

void NajaPythonProperty::onReleasedBy(const NajaObject* owner) {
  if (owner_ == owner) {
    onNotOwned();
  }
}

void NajaPythonProperty::onNotOwned() {
  destroy();
}

void NajaPythonProperty::setOffset(std::ptrdiff_t offset) {
  if (offset_ >= 0) {
    throw NL::NLException(twiceSetOffset); // LCOV_EXCL_LINE
  }

  offset_ = offset;
}

std::string NajaPythonProperty::getString() const {
  std::ostringstream os;
  return std::string();
}

}  // namespace naja
