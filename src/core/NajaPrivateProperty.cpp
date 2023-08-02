// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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
