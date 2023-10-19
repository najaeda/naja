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
    using Inherit = naja::NajaPrivateProperty;
    static const inline std::string Name = "SNLDesignModelingProperty";
    static SNLDesignModelingProperty* create(naja::SNL::SNLDesign* design) {
      preCreate(design, Name);
      SNLDesignModelingProperty* property = new SNLDesignModelingProperty();
      property->modeling_ = new naja::SNL::SNLDesignModeling();
      property->postCreate(design);
      return property;
    }
    static void preCreate(naja::SNL::SNLDesign* design, const std::string& name) {
      Inherit::preCreate(design, name);
      if (not (design->isLeaf())) {
        throw naja::SNL::SNLException("");
      }
    }
    std::string getName() const override {
      return Name;
    }
    std::string getString() const override {
      return Name;
    }
    naja::SNL::SNLDesignModeling* getModeling() {
      return modeling_;
    }
  private:
    naja::SNL::SNLDesignModeling* modeling_ {nullptr};
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

void SNLDesignModeling::addCombinatorialDependency_(const SNLBitTerm* input, const SNLBitTerm* output) {
  auto iit = inputCombinatorialDependencies_.find(input);
  if (iit == inputCombinatorialDependencies_.end()) {
    auto result = inputCombinatorialDependencies_.insert({input, TermDependencies()});
    if (not result.second) {
      throw SNLException("");
    }
    iit = result.first;
  }
  TermDependencies& dependencies = iit->second;
  auto oit = dependencies.find(output);
  if (oit != dependencies.end()) {

  }
  dependencies.insert(output);
}

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

void SNLDesignModeling::addCombinatorialDependency(const SNLBitTerm* input, const SNLBitTerm* output) {
  if (not input or not output) {
    throw SNLException("");
  }
  if (input->getDesign() not_eq output->getDesign()) {
    throw SNLException("");
  }
  auto design = input->getDesign();
  auto property = getOrCreateProperty(design);
  auto modeling = property->getModeling();
  modeling->addCombinatorialDependency_(input, output);
}



}} // namespace SNL // namespace naja
