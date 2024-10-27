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
    using Attribute = std::pair<naja::SNL::SNLName, std::string>;
    using Attributes = std::vector<Attribute>;
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
    static SNLAttributesPrivateProperty* getOrCreate(naja::SNL::SNLObject* object) {
      auto prop = object->getProperty(SNLAttributesPrivateProperty::Name);
      if (prop == nullptr) {
        prop = SNLAttributesPrivateProperty::create(object);
      }
      return static_cast<SNLAttributesPrivateProperty*>(prop);
    }
    void addAttribute(const naja::SNL::SNLAttributes::SNLAttribute& attribute) {
      //attributes_
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

}} // namespace SNL // namespace naja