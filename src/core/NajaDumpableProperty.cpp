// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaDumpableProperty.h"
#include "NajaObject.h"
#include "NajaException.h"

namespace naja {

NajaDumpableProperty::NajaDumpableProperty(const std::string& name):
  super(),
  name_(name)
{}

NajaDumpableProperty* NajaDumpableProperty::create(NajaObject* owner, const std::string& name) {
  super::preCreate(owner, name);
  NajaDumpableProperty* property = new NajaDumpableProperty(name);
  property->postCreate(owner);
  return property;
}

//LCOV_EXCL_START
std::string NajaDumpableProperty::getString() const {
  return std::string();
}
//LCOV_EXCL_STOP

} // namespace naja