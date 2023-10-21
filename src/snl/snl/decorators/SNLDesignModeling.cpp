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

void insertInArcs(
  naja::SNL::SNLDesignModeling::Arcs& combinatorialArcs,
  naja::SNL::SNLBitTerm* term0,
  naja::SNL::SNLBitTerm* term1) {
  auto iit = combinatorialArcs.find(term0);
  if (iit == combinatorialArcs.end()) {
    auto result = combinatorialArcs.insert({term0, naja::SNL::SNLDesignModeling::TermArcs()});
    if (not result.second) {
      throw naja::SNL::SNLException("");
    }
    iit = result.first;
  }
  naja::SNL::SNLDesignModeling::TermArcs& arcs = iit->second;
  auto oit = arcs.find(term1);
  if (oit != arcs.end()) {

  }
  arcs.insert(term1);
}
  
}

namespace naja { namespace SNL {

void SNLDesignModeling::addCombinatorialArcs_(SNLBitTerm* input, SNLBitTerm* output) {
  insertInArcs(inputCombinatorialArcs_, input, output);
  insertInArcs(outputCombinatorialArcs_, output, input);
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs_(SNLBitTerm* term) const {
  Arcs::const_iterator it = inputCombinatorialArcs_.find(term);
  if (it == inputCombinatorialArcs_.end()) {
    return NajaCollection<SNLBitTerm*>();
  }
  return NajaCollection(new NajaSTLCollection(&(it->second)));
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs_(SNLBitTerm* term) const {
  Arcs::const_iterator it = outputCombinatorialArcs_.find(term);
  if (it == outputCombinatorialArcs_.end()) {
    return NajaCollection<SNLBitTerm*>();
  }
  return NajaCollection(new NajaSTLCollection(&(it->second)));
}

void SNLDesignModeling::addCombinatorialArcs(
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
      modeling->addCombinatorialArcs_(input, output);
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