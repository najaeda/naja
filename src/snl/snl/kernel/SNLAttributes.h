// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_ATTRIBUTES_H_
#define __SNL_ATTRIBUTES_H_

#include "NajaCollection.h"
#include "SNLName.h"

namespace naja { namespace SNL {

class SNLObject;
class SNLDesign;
class SNLDesignObject;

class SNLAttributes {
  public:
    class SNLAttribute {
      public:
        SNLAttribute() = default;
        SNLAttribute(const SNLName& name, const std::string& value=std::string());
        SNLAttribute(const SNLAttribute&) = default;
        SNLName getName() const { return name_; }
        std::string getValue() const { return value_; }
        bool hasValue() const { return not value_.empty(); }
        bool operator==(const SNLAttribute& ra) const {
          return name_ == ra.name_ and value_ == ra.value_;
        };
      private:
        SNLName     name_   {};
        std::string value_  {};
    };
    static void addAttribute(SNLDesign* design, const SNLAttribute& attribute);
    static void addAttribute(SNLDesignObject* designObject, const SNLAttribute& attribute);
    static void clearAttributes(SNLObject* object);
    static NajaCollection<SNLAttribute> getAttributes(const SNLObject* object);
};

}} // namespace SNL // namespace naja

#endif // __SNL_ATTRIBUTES_H_