// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaObject.h"

#include "NajaException.h"
#include "NajaDumpableProperty.h"

namespace naja {

void NajaObject::destroy() {
  preDestroy();
  delete this;
}

NajaProperty* NajaObject::getProperty(const std::string& name) const {
  auto it = properties_.find(name);
  if (it != properties_.end()) {
    return it->second;
  }
  return nullptr;
}

void NajaObject::addProperty(NajaProperty* property) {
  //No collision verification as only Private properties are implemented
  //This might need to be refined in the future
  properties_[property->getName()] = property;
  property->onCapturedBy(this);
}

void NajaObject::removeProperty(NajaProperty* property) {
  auto it = properties_.find(property->getName());
  assert(it != properties_.end());
  properties_.erase(it);
}

NajaCollection<NajaProperty*> NajaObject::getProperties() const {
  return NajaCollection(new NajaSTLMapCollection(&properties_));
}

NajaCollection<NajaDumpableProperty*> NajaObject::getDumpableProperties() const {
  return getProperties().getSubCollection<NajaDumpableProperty*>();
}

void NajaObject::preDestroy() {
  for (auto it=properties_.begin(); it!=properties_.end();) {
    NajaProperty* property = it->second;
    ++it;
    property->onReleasedBy(this);
  }
}

void NajaObject::put(NajaProperty* property) {
  if (!property) {
    std::string reason =
        "NajaObject::remove(): Can't remove property : NULL property.";
    throw NajaException(reason);
  }
  NajaProperty* oldProperty = getProperty(property->getName());
  if (property != oldProperty) {
    if (oldProperty) {
      removeProperty(oldProperty);
      oldProperty->onReleasedBy(this);
    }
    addProperty(property);
  }
}

void NajaObject::remove(NajaProperty* property) {
  if (!property) {
    std::string reason =
        "NajaObject::remove(): Can't remove property : NULL property.";
    throw NajaException(reason);
  }
  if (properties_.find(property->getName()) != properties_.end()) {
    removeProperty(property);
    property->onReleasedBy(this);
  }
}

void NajaObject::onDestroyed(NajaProperty* property) {
  if (properties_.find(property->getName()) != properties_.end()) {
    removeProperty(property);
  }
}

}  // namespace naja