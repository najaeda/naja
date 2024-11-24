// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLProxyProperty.h"
#include "NajaException.h"
#include "NajaProperty.h"
#include "SNLException.h"
// Include for ofstraem
#include <sstream>
#include "NajaObject.h"

namespace naja {

// ---------------------------------------------------------------
// Local Variables.

const char* twiceSetOffset =
    "SNLProxyProperty::setOffset () :\n\n"
    "    Attempt to sets the _shadow member offset twice.\n";

// -------------------------------------------------------------------
// Class : "SNLProxyProperty".

std::string SNLProxyProperty::_name = "Isobar::SNLProxyProperty";
int SNLProxyProperty::_offset = -1;

SNLProxyProperty::SNLProxyProperty(void* shadow)
    : NajaPrivateProperty(), _owner  (NULL)
      ,
      _shadow(shadow) {}

SNLProxyProperty* SNLProxyProperty::create(void* shadow) {
  if (not shadow)
    throw SNL::SNLException(
        "SNLProxyProperty::create(): Empty \"shadow\" argument.");

  SNLProxyProperty* property = new SNLProxyProperty(shadow);

  if (not property)
    throw SNL::SNLException("SNLProxyProperty::create()");

  return property;
}

void SNLProxyProperty::preDestroy() {
  if (_owner) {
    _owner->onDestroyed(this);
  }
  if (_offset > 0) {
    void** shadowMember = ((void**)((unsigned long)_shadow + _offset));
    *shadowMember = NULL;
  }
}

void SNLProxyProperty::onCapturedBy(NajaObject* owner) {
  if ((_owner != NULL) && (_owner != owner))
    throw SNL::SNLException(getString().c_str());

  _owner = owner;
}

void SNLProxyProperty::onReleasedBy(const NajaObject* owner) {
  if (_owner == owner)
    onNotOwned();
}

void SNLProxyProperty::onNotOwned() {
  destroy();
}

void SNLProxyProperty::setOffset(int offset) {
  if (_offset >= 0)
    throw SNL::SNLException(twiceSetOffset);

  _offset = offset;
}

std::string SNLProxyProperty::getString() const {
  std::ostringstream os;
  return std::string();
}

}  // namespace naja