// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaRelationProperty.h"

#include "NajaException.h"
#include "NajaObject.h"

namespace naja {

NajaRelationProperty* NajaRelationProperty::create(NajaObject* owner, const std::string& name) {
  if (!owner) {
    throw NajaException("NajaRelationProperty::create(): owner is null");
  }
  NajaRelationProperty* relation = new NajaRelationProperty(name, owner);
  owner->put(relation);
  return relation;
}

std::string NajaRelationProperty::getString() const {
  return name_ + " (owner: " + owner_->getTypeName()
    + ", " + std::to_string(members_.size()) + " members)";
}

void NajaRelationProperty::join(NajaObject* object) {
  object->put(this);
}

void NajaRelationProperty::leave(NajaObject* object) {
  if (object == owner_) {
    throw NajaException("NajaRelationProperty::leave(): the owner cannot leave its own relation");
  }
  object->remove(this);
}

void NajaRelationProperty::preDestroy() {
  super::preDestroy();
  Members members;
  members.swap(members_);
  //removeProperty() directly rather than remove(): remove() would call back
  //into onReleasedBy() and destroy this relation a second time.
  for (const NajaObject* member : members) {
    const_cast<NajaObject*>(member)->removeProperty(this);
  }
  if (owner_ && owner_->getProperty(name_) == this) {
    owner_->removeProperty(this);
  }
}

void NajaRelationProperty::onCapturedBy(NajaObject* object) {
  if (object != owner_) {
    members_.insert(object);
  }
}

void NajaRelationProperty::onReleasedBy(const NajaObject* object) {
  if (object == owner_) {
    destroy();
  } else {
    members_.erase(object);
  }
}

} // namespace naja
