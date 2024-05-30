// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaDumpableProperty.h"
#include "NajaObject.h"
#include "NajaException.h"

namespace naja {

NajaDumpableProperty::NajaDumpableProperty(const std::string& name):
  super(),
  name_(name),
  values_()
{}

NajaDumpableProperty* NajaDumpableProperty::create(NajaObject* owner, const std::string& name) {
  super::preCreate(owner, name);
  NajaDumpableProperty* property = new NajaDumpableProperty(name);
  property->postCreate(owner);
  return property;
}

std::string NajaDumpableProperty::getStringValue(size_t i) const {
  if (i >= values_.size()) {
    throw NajaException("NajaDumpableProperty::getStringValue: index out of range");
  }
  if (values_[i].index() != NajaDumpableProperty::String) {
    throw NajaException("NajaDumpableProperty::getStringValue: type is not String");
  }
  return std::get<NajaDumpableProperty::String>(values_[i]);
}

void NajaDumpableProperty::addStringValue(const std::string& value) {
  values_.push_back(Value(value));
}

void NajaDumpableProperty::addUInt64Value(uint64_t value) {
  values_.push_back(Value(value));
}

uint64_t NajaDumpableProperty::getUInt64Value(size_t i) const {
  if (i >= values_.size()) {
    throw NajaException("NajaDumpableProperty::getUInt64Value: index out of range");
  }
  if (values_[i].index() != NajaDumpableProperty::UInt64) {
    throw NajaException("NajaDumpableProperty::getUInt64Value: type is not UInt64");
  }
  return std::get<NajaDumpableProperty::UInt64>(values_[i]);
}

//LCOV_EXCL_START
std::string NajaDumpableProperty::getString() const {
  return std::string();
}
//LCOV_EXCL_STOP

} // namespace naja