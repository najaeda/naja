// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_ATTRIBUTES_H_
#define __SNL_ATTRIBUTES_H_

#include "NajaCollection.h"
#include "NLName.h"

namespace naja { namespace NL {

class NLObject;
class SNLDesign;
class SNLDesignObject;

class SNLAttributeValue {
  public:
    enum class Type { NUMBER, STRING };
    SNLAttributeValue() = default;
    SNLAttributeValue(const SNLAttributeValue&) = default;
    explicit SNLAttributeValue(const std::string& value): type_(Type::STRING), value_(value) {};
    SNLAttributeValue(Type type, const std::string& value): type_(type), value_(value) {};
    std::string getString() const { return value_; }
    bool isString() const { return type_ == Type::STRING; }
    bool empty() const { return value_.empty(); }
    bool operator==(const SNLAttributeValue& rv) const = default;
    bool operator<(const SNLAttributeValue& rv) const {
      if (type_ == rv.type_) {
        return value_ < rv.value_;
      }
      return type_ < rv.type_;
    }
    bool operator<=(const SNLAttributeValue& rv) const {
      return *this < rv or *this == rv;
    } 
    bool operator>(const SNLAttributeValue& rv) const {
      if (type_ == rv.type_) {
        return value_ > rv.value_;
      }
      return type_ > rv.type_;
    }
    bool operator>=(const SNLAttributeValue& rv) const {
      return *this > rv or *this == rv;
    }
  private:
    Type        type_   {Type::STRING};
    std::string value_  {};
};

class SNLAttribute {
  public:
    //Values are either numbers or strings.
    //but stored as strings.
    SNLAttribute() = default; //LCOV_EXCL_LINE
    explicit SNLAttribute(const NLName& name, const SNLAttributeValue& value=SNLAttributeValue());
    SNLAttribute(const SNLAttribute&) = default;
    
    NLName getName() const { return name_; }
    SNLAttributeValue getValue() const { return value_; }
    std::string getString() const;
    bool hasValue() const { return not value_.empty(); }
    bool operator==(const SNLAttribute& ra) const {
      return name_ == ra.name_ and value_ == ra.value_;
    };
    bool operator<(const SNLAttribute& ra) const {
      if (name_ == ra.name_) {
        return value_ < ra.value_;
      }
      return name_ < ra.name_;
    }
    bool operator<=(const SNLAttribute& ra) const {
      return *this < ra or *this == ra;
    }
    bool operator>(const SNLAttribute& ra) const {
      if (name_ == ra.name_) {
        return value_ > ra.value_;
      }
      return name_ > ra.name_;
    }
    bool operator>=(const SNLAttribute& ra) const {
      return *this > ra or *this == ra;
    }
  private:
    NLName           name_    {};
    SNLAttributeValue value_  {};
};

class SNLAttributes {
  public:
    static void addAttribute(SNLDesign* design, const SNLAttribute& attribute);
    static void addAttribute(SNLDesignObject* designObject, const SNLAttribute& attribute);
    static void clearAttributes(NLObject* object);
    static void cloneAttributes(const NLObject* from, NLObject* to);
    static bool compareAttributes(const NLObject* object1, const NLObject* object2, std::string& reason);
    static NajaCollection<SNLAttribute> getAttributes(const NLObject* object);
};

}} // namespace NL // namespace naja

#endif // __SNL_ATTRIBUTES_H_
