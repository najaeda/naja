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

NajaCollection<NajaProperty*> NajaObject::getProperties() const {
  return NajaCollection(new NajaSTLMapCollection(&properties_));
}

void NajaObject::preDestroy() {
  for (const auto& propertiesPair: properties_) {
    propertiesPair.second->onReleasedBy(this);
  }
  properties_.clear();
}

} // namespace naja