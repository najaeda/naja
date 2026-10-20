// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaSharedProperty.h"

#include "NajaObject.h"

namespace naja {

NajaSharedProperty* NajaSharedProperty::create(const std::string& name) {
  return new NajaSharedProperty(name);
}

std::string NajaSharedProperty::getString() const {
  return name_ + " (" + std::to_string(members_.size()) + " members)";
}

void NajaSharedProperty::join(NajaObject* object) {
  object->put(this);
}

void NajaSharedProperty::leave(NajaObject* object) {
  object->remove(this);
}

void NajaSharedProperty::preDestroy() {
  super::preDestroy();
  Members members;
  members.swap(members_);
  //removeProperty() directly rather than remove(): remove() would call back
  //into onReleasedBy() and destroy this property a second time.
  for (const NajaObject* member : members) {
    const_cast<NajaObject*>(member)->removeProperty(this);
  }
}

void NajaSharedProperty::onCapturedBy(NajaObject* object) {
  members_.insert(object);
}

void NajaSharedProperty::onReleasedBy(const NajaObject* object) {
  members_.erase(object);
  if (members_.empty()) {
    destroy();
  }
}

} // namespace naja
