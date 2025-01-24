// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#include "SNLAttributes.h"

#include <sstream>

#include "NajaPrivateProperty.h"
#include "SNLDesign.h"

namespace {

class SNLAttributesPrivateProperty: public naja::NajaPrivateProperty {
  public:
    using Inherit = naja::NajaPrivateProperty;
    using SNLAttribute = naja::SNL::SNLAttribute;
    using Attributes = std::vector<SNLAttribute>;
    static const inline std::string Name = "SNLDesignTruthTableProperty";
    
    static SNLAttributesPrivateProperty* create(naja::SNL::SNLObject* object) {
      preCreate(object, Name);
      auto property = new SNLAttributesPrivateProperty();
      property->postCreate(object);
      return property;
    }
    
    std::string getName() const override {
      return Name;
    }

    //LCOV_EXCL_START
    std::string getString() const override {
      return Name;
    }
    //LCOV_EXCL_STOP
    
    static SNLAttributesPrivateProperty* get(const naja::SNL::SNLObject* object) {
      return static_cast<SNLAttributesPrivateProperty*>(
        object->getProperty(SNLAttributesPrivateProperty::Name)
      );
    }

    static SNLAttributesPrivateProperty* getOrCreate(naja::SNL::SNLObject* object) {
      auto prop = get(object);
      if (prop == nullptr) {
        prop = SNLAttributesPrivateProperty::create(object);
      }
      return prop;
    }

    void addAttribute(const SNLAttribute& attribute) {
      attributes_.push_back(attribute);
    }

    naja::NajaCollection<SNLAttribute> getAttributes() const {
      return naja::NajaCollection(new naja::NajaSTLCollection(&attributes_));
    }

  private:
    Attributes  attributes_;
};

}

namespace naja { namespace SNL {

SNLAttribute::SNLAttribute(
  const SNLName& name,
  const SNLAttributeValue& value):
  name_(name), value_(value)
{}

//LCOV_EXCL_START
std::string SNLAttribute::getString() const {
  std::string str;
  str += name_.getString();
  if (not value_.empty()) {
    str += " = ";
    if (value_.isString()) {
      str += "\"";
    }
    str += value_.getString();
    if (value_.isString()) {
      str += "\"";
    }
  }
  return str;
}
//LCOV_EXCL_STOP

void SNLAttributes::addAttribute(SNLDesign* design, const SNLAttribute& attribute) {
  auto prop = SNLAttributesPrivateProperty::getOrCreate(design);
  prop->addAttribute(attribute);
}

void SNLAttributes::addAttribute(SNLDesignObject* object, const SNLAttribute& attribute) {
  auto prop = SNLAttributesPrivateProperty::getOrCreate(object);
  prop->addAttribute(attribute);
}

NajaCollection<SNLAttribute> SNLAttributes::getAttributes(const SNLObject* object) {
  auto prop = SNLAttributesPrivateProperty::get(object);
  if (prop) {
    return prop->getAttributes();
  }
  return NajaCollection<SNLAttribute>();
}

void SNLAttributes::cloneAttributes(const SNLObject* from, SNLObject* to) {
  auto propFrom = SNLAttributesPrivateProperty::get(from);
  if (propFrom) {
    auto propTo = SNLAttributesPrivateProperty::getOrCreate(to);
    for (auto attribute: propFrom->getAttributes()) {
      propTo->addAttribute(attribute);
    }
  }
}

bool SNLAttributes::compareAttributes(
  const SNLObject* object1,
  const SNLObject* object2,
  std::string& reason) {
  auto prop1 = SNLAttributesPrivateProperty::get(object1);
  auto prop2 = SNLAttributesPrivateProperty::get(object2);
  //xor
  if ((prop1 and not prop2) or (not prop1 and prop2)) {
    std::ostringstream oss;
    oss << "attributes property mismatch between ";
    oss << object1->getDescription() << " and " << object2->getDescription();
    reason = oss.str();
    return false;
  }
  if (prop1 and prop2) {
    using Attributes = std::vector<SNLAttribute>;
    Attributes attributes1(
      prop1->getAttributes().begin(),
      prop1->getAttributes().end());
    Attributes attributes2(
      prop2->getAttributes().begin(),
      prop2->getAttributes().end());
    if (attributes1.size() not_eq attributes2.size()) {
      reason = "attributes size mismatch";
      return false;
    }
    for (size_t i=0; i<attributes1.size(); i++) {
      if (attributes1[i] not_eq attributes2[i]) {
        reason = "attribute mismatch";
        return false;
      }
    }
  }
  return true;
}

void SNLAttributes::clearAttributes(SNLObject* object) {
  auto prop = SNLAttributesPrivateProperty::get(object);
  if (prop) {
    prop->destroy();
  }
}

}} // namespace SNL // namespace naja