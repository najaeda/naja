// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#include "SNLAttributes.h"

#include "NajaPrivateProperty.h"
#include "SNLDesign.h"

namespace {

class SNLAttributesPrivateProperty: public naja::NajaPrivateProperty {
  public:
    using Inherit = naja::NajaPrivateProperty;
    using SNLAttribute = naja::SNL::SNLAttributes::SNLAttribute;
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

SNLAttributes::SNLAttribute::SNLAttribute(
  const SNLName& name,
  const std::string& value):
  name_(name), value_(value)
{}

void SNLAttributes::addAttribute(SNLDesign* design, const SNLAttribute& attribute) {
  auto prop = SNLAttributesPrivateProperty::getOrCreate(design);
  prop->addAttribute(attribute);
}

void SNLAttributes::addAttribute(SNLDesignObject* object, const SNLAttribute& attribute) {
  auto prop = SNLAttributesPrivateProperty::getOrCreate(object);
  prop->addAttribute(attribute);
}

NajaCollection<SNLAttributes::SNLAttribute> SNLAttributes::getAttributes(const SNLObject* object) {
  auto prop = SNLAttributesPrivateProperty::get(object);
  if (prop) {
    return prop->getAttributes();
  }
  return NajaCollection<SNLAttributes::SNLAttribute>();
}

void SNLAttributes::clearAttributes(SNLObject* object) {
  auto prop = SNLAttributesPrivateProperty::get(object);
  if (prop) {
    prop->destroy();
  }
}

}} // namespace SNL // namespace naja