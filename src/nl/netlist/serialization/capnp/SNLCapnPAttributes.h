// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "SNLAttributes.h"
#include "NLException.h"

namespace naja::NL {

template<typename Builder>
void dumpAttributes(Builder builder, const NLObject* object) {
  auto attributes = SNLAttributes::getAttributes(object);
  if (attributes.empty()) return;
  auto builders = builder.initAttributes(attributes.size());
  size_t id = 0;
  for (const auto& attribute: attributes) {
    auto entry = builders[id++];
    entry.setName(attribute.getName().getString());
    auto value = attribute.getValue();
    if (value.isNumber()) {
      entry.setNumber(value.getString());
    } else if (attribute.hasValue()) {
      entry.setString(value.getString());
    } else {
      // SNL represents valueless attributes and empty strings identically.
      entry.setNone();
    }
  }
}

template<typename Reader, typename Object>
void loadAttributes(Reader reader, Object* object) {
  for (auto entry: reader.getAttributes()) {
    SNLAttributeValue value;
    if (entry.isNumber()) {
      value = SNLAttributeValue(SNLAttributeValue::Type::NUMBER, entry.getNumber());
    } else if (entry.isString()) {
      value = SNLAttributeValue(std::string(entry.getString()));
    } else if (not entry.isNone()) {
      throw NLException("cannot deserialize attribute: unsupported value type");
    }
    SNLAttributes::addAttribute(object, SNLAttribute(NLName(entry.getName()), value));
  }
}

} // namespace naja::NL
