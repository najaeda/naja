// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaDumpableProperty.h"
#include "NajaObject.h"
#include "NajaException.h"

namespace naja {

NajaDumpableProperty::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string NajaDumpableProperty::Type::getString() const {
  switch (typeEnum_) {
    case Type::String: return "String";
    case Type::UInt64:return "UInt64";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

NajaDumpableProperty::NajaDumpableProperty(const std::string& name):
  super(),
  name_(name),
  type_(Type::String),
  value_()
{}

NajaDumpableProperty* NajaDumpableProperty::create(NajaObject* owner, const std::string& name) {
  super::preCreate(owner, name);
  NajaDumpableProperty* property = new NajaDumpableProperty(name);
  property->postCreate(owner);
  return property;
}

std::string NajaDumpableProperty::getStringValue() const {
  if (type_ != Type::String) {
    throw NajaException("NajaDumpableProperty::getStringValue: type is not String");
  }
  return std::get<Type::String>(value_);
}

void NajaDumpableProperty::setStringValue(const std::string& value) {
  type_ = Type::String;
  value_ = value;
}

void NajaDumpableProperty::setUInt64Value(uint64_t value) {
  type_ = Type::UInt64;
  value_ = value;
}

uint64_t NajaDumpableProperty::getUInt64Value() const {
  if (type_ != Type::UInt64) {
    throw NajaException("NajaDumpableProperty::getUInt64Value: type is not UInt64");
  }
  return std::get<Type::UInt64>(value_);
}

//LCOV_EXCL_START
std::string NajaDumpableProperty::getString() const {
  return std::string();
}
//LCOV_EXCL_STOP

} // namespace naja