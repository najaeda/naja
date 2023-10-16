// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignModeling.h"

#include "NajaPrivateProperty.h"
#include "SNLDesign.h"
#include "SNLException.h"

namespace {

class SNLDesignModelingProperty: public naja::NajaPrivateProperty {
  public:
    static const inline std::string Name = "SNLDesignModelingProperty";
    static SNLDesignModelingProperty* create(naja::SNL::SNLDesign* design) {
      preCreate(design, Name);
      SNLDesignModelingProperty* property = new SNLDesignModelingProperty();
      property->postCreate(design);
      return property;
    }
    std::string getName() const override {
      return Name;
    }
    std::string getString() const override {
      return Name;
    }
  private:
};

SNLDesignModelingProperty* getProperty(const naja::SNL::SNLDesign* design) {
  auto property = design->getProperty(SNLDesignModelingProperty::Name);
  if (property) {
    return static_cast<SNLDesignModelingProperty*>(property);
  }
  return nullptr;
}

SNLDesignModelingProperty* getOrCreateProperty(naja::SNL::SNLDesign* design) {
  auto property = getProperty(design);
  if (property) {
    return property;
  } 
  return SNLDesignModelingProperty::create(design);
}
  
}

namespace naja { namespace SNL {

#if 0
void SNLDesignModelingDecorator::setValue(SNLDesign* design, int value) {
  auto property = getOrCreateSNLDesignModelingProperty(design);
  property->setValue(value);
}

int SNLDesignModelingDecorator::getValue(const SNLDesign* design) {
  auto property = getSNLDesignModelingProperty(design);
  if (property) {
    return property->getValue();
  }
  //best use case if property holds a pointer to something for property existence
  return 0;
}

#endif

void SNLDesignModeling::addTimingArc(const SNLTerm* input, const SNLTerm* output) {
  if (not input or not output) {
    throw SNLException("");
  }
  if (input->getDesign() not_eq output->getDesign()) {
    throw SNLException("");
  }
  auto design = input->getDesign();
  auto property = getOrCreateProperty(design);
  property->getString();
}



}} // namespace SNL // namespace naja
