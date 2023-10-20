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
    void preDestroy() override {
      if (modeling_) {
        delete modeling_;
      }
      Inherit::preDestroy();
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

void insertInDependencies(
  naja::SNL::SNLDesignModeling::Dependencies& combinatorialDependencies,
  naja::SNL::SNLBitTerm* term0,
  naja::SNL::SNLBitTerm* term1) {
  auto iit = combinatorialDependencies.find(term0);
  if (iit == combinatorialDependencies.end()) {
    auto result = combinatorialDependencies.insert({term0, naja::SNL::SNLDesignModeling::TermDependencies()});
    if (not result.second) {
      throw naja::SNL::SNLException("");
    }
    iit = result.first;
  }
  naja::SNL::SNLDesignModeling::TermDependencies& dependencies = iit->second;
  auto oit = dependencies.find(term1);
  if (oit != dependencies.end()) {

  }
  dependencies.insert(term1);
}
  
}

namespace naja { namespace SNL {

void SNLDesignModeling::addCombinatorialDependency_(SNLBitTerm* input, SNLBitTerm* output) {
  insertInDependencies(inputCombinatorialDependencies_, input, output);
  insertInDependencies(outputCombinatorialDependencies_, output, input);
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs_(SNLBitTerm* term) const {
  Dependencies::const_iterator it = inputCombinatorialDependencies_.find(term);
  if (it == inputCombinatorialDependencies_.end()) {
    return NajaCollection<SNLBitTerm*>();
  }
  return NajaCollection(new NajaSTLCollection(&(it->second)));
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs_(SNLBitTerm* term) const {
  Dependencies::const_iterator it = outputCombinatorialDependencies_.find(term);
  if (it == outputCombinatorialDependencies_.end()) {
    return NajaCollection<SNLBitTerm*>();
  }
  return NajaCollection(new NajaSTLCollection(&(it->second)));
}

void SNLDesignModeling::addCombinatorialDependency(
  const BitTerms& inputs, const BitTerms& outputs) {
  if (inputs.empty()) {
    throw SNLException("Error in addCombinatorialDependency: empty inputs");
  }
  if (outputs.empty()) {
    throw SNLException("Error in addCombinatorialDependency: empty outputs");
  }
  SNLDesign* design = nullptr;
  for (auto input: inputs) {
    if (not design) {
      design = input->getDesign();
    }
    if (design not_eq input->getDesign()) {
      throw SNLException("Error in addCombinatorialDependency: incompatible designs");
    }
    for (auto output: outputs) {
      if (design not_eq output->getDesign()) {
        throw SNLException("Error in addCombinatorialDependency: incompatible designs");
      }
    }
  }
  auto property = getOrCreateProperty(design);
  auto modeling = property->getModeling();
  for (auto input: inputs) {
    for (auto output: outputs) {
      modeling->addCombinatorialDependency_(input, output);
    }
  }
}

NajaCollection<SNLBitTerm*>  SNLDesignModeling::getCombinatorialOutputs(SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    auto modeling = property->getModeling();
    return modeling->getCombinatorialOutputs_(term);
  }
  return NajaCollection<SNLBitTerm*>();
}

NajaCollection<SNLBitTerm*>  SNLDesignModeling::getCombinatorialInputs(SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    auto modeling = property->getModeling();
    return modeling->getCombinatorialInputs_(term);
  }
  return NajaCollection<SNLBitTerm*>();
}



}} // namespace SNL // namespace naja
