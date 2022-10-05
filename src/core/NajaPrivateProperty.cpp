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

#include "NajaPrivateProperty.h"
#include "NajaObject.h"
#include "NajaException.h"

namespace naja {

void NajaPrivateProperty::preCreate(const NajaObject* object, const std::string& name) {
  if (object->hasProperty(name)) {
    std::string reason = "cannot create NajaPrivateProperty with name " + name;
    reason += "A property with this name already exists.";
    throw NajaException(reason);
  }
}

void NajaPrivateProperty::postCreate(NajaObject* owner) {
  super::postCreate();
  owner->addProperty(this);
}

void NajaPrivateProperty::preDestroy() {
  super::preDestroy();
  if (owner_) {
    owner_->removeProperty(this);
  }
}

void NajaPrivateProperty::onCapturedBy(NajaObject* object) {
  owner_ = object;
}

void NajaPrivateProperty::onReleasedBy(const NajaObject* object) {
  if (owner_ == object) {
    destroy();
  }
} 

} // namespace naja
