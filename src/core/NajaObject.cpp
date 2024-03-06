// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaObject.h"

#include "NajaException.h"
#include "NajaProperty.h"

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

NajaCollection<NajaProperty*> NajaObject::getDumpableProperties() const {
  auto filter = [](const NajaProperty* p) { return p->isDumpable(); };
  return getProperties().getSubCollection(filter);
}

void NajaObject::preDestroy() {
  for (auto it=properties_.begin(); it!=properties_.end();) {
    NajaProperty* property = it->second;
    ++it;
    property->onReleasedBy(this);
  }
}

} // namespace naja
