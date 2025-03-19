// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaPythonProperty.h"

#include <sstream>

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
int NajaPythonProperty::offset_ = -1;

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
  if (offset_ > 0) {
    void** shadowMember = ((void**)((unsigned long)shadow_ + offset_));
    *shadowMember = nullptr;
  }
}

void NajaPythonProperty::onCapturedBy(NajaObject* owner) {
  if (owner_ and (owner_ != owner))
    throw NL::NLException(getString().c_str());

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

void NajaPythonProperty::setOffset(int offset) {
  if (offset_ >= 0)
    throw NL::NLException(twiceSetOffset);

  offset_ = offset;
}

std::string NajaPythonProperty::getString() const {
  std::ostringstream os;
  return std::string();
}

}  // namespace naja
