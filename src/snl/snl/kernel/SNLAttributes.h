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
        //Values are either numbers or strings.
        //but stored as strings.
        class Value {
          public:
            enum class Type { NUMBER, STRING };
            Value();
            Value(const std::string& value): type_(Type::STRING), value_(value) {};
            Value(Type type, const std::string& value): type_(type), value_(value) {};
            Value(const Value&) = default;
            std::string getString() const { return value_; }
            bool isString() const { return type_ == Type::STRING; }
            bool empty() const { return value_.empty(); }
            bool operator==(const Value& rv) const = default;
          private:
            Type        type_   { Type::STRING };
            std::string value_  {};
        };

        SNLAttribute() = default;
        SNLAttribute(const SNLName& name, const SNLAttribute::Value& value=SNLAttribute::Value());
        SNLAttribute(const SNLAttribute&) = default;
        SNLName getName() const { return name_; }
        Value getValue() const { return value_; }
        std::string getString() const;
        bool hasValue() const { return not value_.empty(); }
        bool operator==(const SNLAttribute& ra) const {
          return name_ == ra.name_ and value_ == ra.value_;
        };
      private:
        SNLName     name_   {};
        Value       value_  {};
    };
    static void addAttribute(SNLDesign* design, const SNLAttribute& attribute);
    static void addAttribute(SNLDesignObject* designObject, const SNLAttribute& attribute);
    static void clearAttributes(SNLObject* object);
    static void cloneAttributes(const SNLObject* from, SNLObject* to);
    static bool compareAttributes(const SNLObject* object1, const SNLObject* object2, std::string& reason);
    static NajaCollection<SNLAttribute> getAttributes(const SNLObject* object);
};

}} // namespace SNL // namespace naja

#endif // __SNL_ATTRIBUTES_H_