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
    static SNLDesignModelingProperty* create(
      naja::SNL::SNLDesign* design,
      naja::SNL::SNLDesignModeling::Type type) {
      preCreate(design, Name);
      SNLDesignModelingProperty* property = new SNLDesignModelingProperty();
      property->modeling_ = new naja::SNL::SNLDesignModeling(type);
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
    naja::SNL::SNLDesignModeling::Type getModelingType() const {
      return getModeling()->getType();
    }
    std::string getName() const override {
      return Name;
    }
    std::string getString() const override {
      return Name;
    }
    naja::SNL::SNLDesignModeling* getModeling() const {
      return modeling_;
    }
  private:
    naja::SNL::SNLDesignModeling* modeling_ {nullptr};
};

SNLDesignModelingProperty* getProperty(const naja::SNL::SNLDesign* design) {
  auto property =
    static_cast<SNLDesignModelingProperty*>(design->getProperty(SNLDesignModelingProperty::Name));
  if (property) {
    return property;
  }
  return nullptr;
}

SNLDesignModelingProperty* getOrCreateProperty(
  naja::SNL::SNLDesign* design,
  naja::SNL::SNLDesignModeling::Type type) {
  auto property = getProperty(design);
  if (property) {
    return property;
  } 
  return SNLDesignModelingProperty::create(design, type);
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

SNLDesignModeling::SNLDesignModeling(Type type): type_(type) {
  if (type_ == NO_PARAMETER) {
    model_ = TimingArcs(); 
  } else {
    model_ = ParameterizedArcs();
  }
}

void SNLDesignModeling::addCombinatorialArcs_(SNLBitTerm* input, SNLBitTerm* output) {
  if (type_ not_eq Type::NO_PARAMETER) {
    throw SNLException("");
  }
  TimingArcs& arcs = std::get<Type::NO_PARAMETER>(model_);
  insertInArcs(arcs.inputCombinatorialArcs_, input, output);
  insertInArcs(arcs.outputCombinatorialArcs_, output, input);
}

const SNLDesignModeling::TimingArcs* SNLDesignModeling::getTimingArcs() const {
  if (type_ == Type::NO_PARAMETER) {
    return &std::get<Type::NO_PARAMETER>(model_);
  } else {
    const ParameterizedArcs& parameterizedArcs = std::get<Type::PARAMETERIZED>(model_);
    auto ait = parameterizedArcs.find(defaultParameter_);
    if (ait != parameterizedArcs.end()) {
      return &(ait->second);
    } else {
      throw SNLException("");
    }
  }

}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs_(SNLBitTerm* term) const {
  const auto* timingArcs = getTimingArcs();
  Arcs::const_iterator it = timingArcs->inputCombinatorialArcs_.find(term);
  if (it == timingArcs->inputCombinatorialArcs_.end()) {
    return NajaCollection<SNLBitTerm*>();
  }
  return NajaCollection(new NajaSTLCollection(&(it->second)));
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs_(SNLBitTerm* term) const {
  const auto* timingArcs = getTimingArcs();
  Arcs::const_iterator it = timingArcs->outputCombinatorialArcs_.find(term);
  if (it == timingArcs->outputCombinatorialArcs_.end()) {
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
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto input: inputs) {
    for (auto output: outputs) {
      modeling->addCombinatorialArcs_(input, output);
    }
  }
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs(SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    auto modeling = property->getModeling();
    return modeling->getCombinatorialOutputs_(term);
  }
  return NajaCollection<SNLBitTerm*>();
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs(SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    auto modeling = property->getModeling();
    return modeling->getCombinatorialInputs_(term);
  }
  return NajaCollection<SNLBitTerm*>();
}

}} // namespace SNL // namespace naja
