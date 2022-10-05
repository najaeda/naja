/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
