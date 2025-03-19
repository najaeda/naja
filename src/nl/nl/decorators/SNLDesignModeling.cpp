// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignModeling.h"

#include <sstream>

#include "NajaPrivateProperty.h"
#include "NLException.h"

#include "SNLDesign.h"
#include "SNLInstTerm.h"

namespace {

class SNLDesignModelingProperty: public naja::NajaPrivateProperty {
  public:
    using Inherit = naja::NajaPrivateProperty;
    static const inline std::string Name = "SNLDesignModelingProperty";
    static SNLDesignModelingProperty* create(
      naja::NL::SNLDesign* design,
      naja::NL::SNLDesignModeling::Type type) {
      preCreate(design, Name);
      SNLDesignModelingProperty* property = new SNLDesignModelingProperty();
      property->modeling_ = new naja::NL::SNLDesignModeling(type);
      property->postCreate(design);
      return property;
    }
    static void preCreate(naja::NL::SNLDesign* design, const std::string& name) {
      Inherit::preCreate(design, name);
      if (not (design->isLeaf())) {
        std::ostringstream reason;
        reason << "Impossible to add Timing Modeling on a non leaf design <"
          << design->getName().getString() << ">";
        throw naja::NL::NLException(reason.str());
      }
    }
    void preDestroy() override {
      if (modeling_) {
        delete modeling_;
      }
      Inherit::preDestroy();
    }
    naja::NL::SNLDesignModeling::Type getModelingType() const {
      return getModeling()->getType();
    }
    std::string getName() const override {
      return Name;
    }
    //LCOV_EXCL_START
    std::string getString() const override {
      return Name;
    }
    //LCOV_EXCL_STOP
    naja::NL::SNLDesignModeling* getModeling() const {
      return modeling_;
    }
  private:
    naja::NL::SNLDesignModeling* modeling_ {nullptr};
};

SNLDesignModelingProperty* getProperty(const naja::NL::SNLDesign* design) {
  auto property =
    static_cast<SNLDesignModelingProperty*>(design->getProperty(SNLDesignModelingProperty::Name));
  if (property) {
    return property;
  }
  return nullptr;
}

//type will used only if the property is created
SNLDesignModelingProperty* getOrCreateProperty(
  naja::NL::SNLDesign* design,
  naja::NL::SNLDesignModeling::Type type) {
  auto property = getProperty(design);
  if (property) {
    return property;
  } 
  return SNLDesignModelingProperty::create(design, type);
}

void insertInArcs(
  naja::NL::SNLDesignModeling::Arcs& arcs,
  naja::NL::SNLBitTerm* term0,
  naja::NL::SNLBitTerm* term1) {
  auto iit = arcs.find(term0);
  if (iit == arcs.end()) {
    auto result = arcs.insert({term0, naja::NL::SNLDesignModeling::TermArcs()});
    if (not result.second) {
      throw naja::NL::NLException("Error while inserting in timing arcs");
    }
    iit = result.first;
  }
  naja::NL::SNLDesignModeling::TermArcs& termArcs = iit->second;
  auto oit = termArcs.find(term1);
  if (oit != termArcs.end()) {
    throw naja::NL::NLException("Error while inserting in timing arcs");
  }
  termArcs.insert(term1);
}

naja::NL::SNLDesign* verifyInputs(
  const naja::NL::SNLDesignModeling::BitTerms& terms0,
  const std::string& terms0Naming,
  const naja::NL::SNLDesignModeling::BitTerms& terms1,
  const std::string& terms1Naming,
  const std::string& method) {
  if (terms0.empty()) {
    throw naja::NL::NLException("Error in " + method + ": empty " + terms0Naming);
  }
  if (terms1.empty()) {
    throw naja::NL::NLException("Error in " + method + ": empty " + terms1Naming);
  }
  naja::NL::SNLDesign* design = nullptr;
  for (auto term: terms0) {
    if (not design) {
      design = term->getDesign();
    } else if (design not_eq term->getDesign()) {
      throw naja::NL::NLException("Error in " + method + ": incompatible designs");
    }
  }
  for (auto term: terms1) {
    if (design not_eq term->getDesign()) {
      throw naja::NL::NLException("Error in " + method + ": incompatible designs");
    }
  }
  return design;
}

#define GET_RELATED_TERMS_IN_ARCS(ARCS) \
  const auto* timingArcs = getTimingArcs(); \
  const auto it = timingArcs->ARCS.find(term); \
  if (it == timingArcs->ARCS.end()) { \
    return NajaCollection<SNLBitTerm*>(); \
  } \
  return NajaCollection(new NajaSTLCollection(&(it->second)));


#define GET_RELATED_INSTTERMS_IN_ARCS(ARCS) \
  auto instance = iterm->getInstance(); \
  const TimingArcs* timingArcs = getTimingArcs(instance); \
  auto it = timingArcs->ARCS.find(iterm->getBitTerm()); \
  if (it == timingArcs->ARCS.end()) { \
    return NajaCollection<SNLInstTerm*>(); \
  } \
  auto transformer = [=](const SNLBitTerm* term) { return instance->getInstTerm(term); }; \
  return NajaCollection(new NajaSTLCollection(&(it->second))).getTransformerCollection<SNLInstTerm*>(transformer);

#define GET_RELATED_OBJECTS(TYPE, INPUT, DESIGN_GETTER, GETTER) \
  auto property = getProperty(INPUT->DESIGN_GETTER); \
  if (property) { \
    auto modeling = property->getModeling(); \
    return modeling->GETTER(INPUT); \
  } \
  return NajaCollection<TYPE*>();
  
}

namespace naja { namespace NL {

SNLDesignModeling::SNLDesignModeling(Type type): type_(type) {
  if (type_ == NO_PARAMETER) {
    model_ = TimingArcs(); 
  } else {
    model_ = ParameterizedArcs();
  }
}

void SNLDesignModeling::addCombinatorialArc_(SNLBitTerm* input, SNLBitTerm* output) {
  TimingArcs* arcs = getOrCreateTimingArcs();
  insertInArcs(arcs->inputCombinatorialArcs_, input, output);
  insertInArcs(arcs->outputCombinatorialArcs_, output, input);
}

void SNLDesignModeling::addCombinatorialArc_(SNLBitTerm* input, SNLBitTerm* output, const std::string& parameterValue) {
  TimingArcs* arcs = getOrCreateTimingArcs(parameterValue);
  insertInArcs(arcs->inputCombinatorialArcs_, input, output);
  insertInArcs(arcs->outputCombinatorialArcs_, output, input);
}

void SNLDesignModeling::addInputToClockArc_(SNLBitTerm* input, SNLBitTerm* clock) {
  if (type_ not_eq Type::NO_PARAMETER) {
    throw NLException("Wrong SNLDesignModeling type for addInputToClockArc");
  }
  TimingArcs& arcs = std::get<Type::NO_PARAMETER>(model_);
  insertInArcs(arcs.inputToClockArcs_, input, clock);
  insertInArcs(arcs.clockToInputArcs_, clock, input);
}

void SNLDesignModeling::addClockToOutputArc_(SNLBitTerm* clock, SNLBitTerm* output) {
  if (type_ not_eq Type::NO_PARAMETER) {
    throw NLException("Wrong SNLDesignModeling type for addClockToOutputArc");
  }
  TimingArcs& arcs = std::get<Type::NO_PARAMETER>(model_);
  insertInArcs(arcs.outputToClockArcs_, output, clock);
  insertInArcs(arcs.clockToOutputArcs_, clock, output);
}

SNLDesignModeling::TimingArcs* SNLDesignModeling::getOrCreateTimingArcs(const std::string& parameterValue) {
  if (type_ == Type::NO_PARAMETER) {
    if (not parameterValue.empty()) {
      throw NLException("Contradictory type in SNLDesignModeling");
    }
    return &std::get<Type::NO_PARAMETER>(model_);
  } else {
    std::string paramValue = parameter_.second;
    if (not parameterValue.empty()) {
      paramValue = parameterValue;
    }
    ParameterizedArcs& parameterizedArcs = std::get<Type::PARAMETERIZED>(model_);
    auto ait = parameterizedArcs.find(paramValue);
    if (ait == parameterizedArcs.end()) {
      //create it
      auto result = parameterizedArcs.insert({paramValue, TimingArcs()});
      if (not result.second) {
        throw naja::NL::NLException("Error in Timing arcs insertion");
      }
      ait = result.first;
    }
    return &(ait->second);
  }
}

const SNLDesignModeling::TimingArcs* SNLDesignModeling::getTimingArcs(const SNLInstance* instance) const {
  if (type_ == Type::NO_PARAMETER) {
    return &std::get<Type::NO_PARAMETER>(model_);
  } else {
    const ParameterizedArcs& parameterizedArcs = std::get<Type::PARAMETERIZED>(model_);
    //find the parameter value
    if (instance != nullptr) {
      // //get Arcs from parameter
      auto parameter = parameter_.first;
      //find parameter in instance
      auto instParameter = instance->getInstParameter(NLName(parameter));
      if (instParameter) {
        auto value = instParameter->getValue();
        auto pit = parameterizedArcs.find(value);
        if (pit == parameterizedArcs.end()) {
          std::ostringstream reason;
          reason << "Parameter value <" << value << "> for Parameter <" << parameter
            << "> cannot be found in design <" << instance->getModel()->getName().getString()
            << "> modeling. Existing values are: ";
          bool first = true;
          for (auto parcs: parameterizedArcs) {
            if (!first) {
              reason << ", ";
            }
            reason << parcs.first;
            first = false;
          }
          throw NLException(reason.str());
        }
        return &(pit->second);
      }
      //if not found then switch to default parameter
    } 
    auto defaultParameterValue = parameter_.second;
    if (defaultParameterValue.empty()) {
      throw NLException("No Default parameter value while getting Timing Arcs");
    }
    auto ait = parameterizedArcs.find(defaultParameterValue);
    if (ait != parameterizedArcs.end()) {
      return &(ait->second);
    } else {
      std::ostringstream reason;
      reason << "cannot find " << defaultParameterValue << " in parameterized arcs.";
      throw NLException(reason.str());
    }
  }
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs_(SNLBitTerm* term) const {
  GET_RELATED_TERMS_IN_ARCS(inputCombinatorialArcs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialOutputs_(SNLInstTerm* iterm) const {
  GET_RELATED_INSTTERMS_IN_ARCS(inputCombinatorialArcs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs_(SNLBitTerm* term) const {
  GET_RELATED_TERMS_IN_ARCS(outputCombinatorialArcs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialInputs_(SNLInstTerm* iterm) const {
  GET_RELATED_INSTTERMS_IN_ARCS(outputCombinatorialArcs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedInputs_(SNLBitTerm* term) const {
  GET_RELATED_TERMS_IN_ARCS(clockToInputArcs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedOutputs_(SNLBitTerm* term) const {
  GET_RELATED_TERMS_IN_ARCS(clockToOutputArcs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getInputRelatedClocks_(SNLBitTerm* term) const {
  GET_RELATED_TERMS_IN_ARCS(inputToClockArcs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getOutputRelatedClocks_(SNLBitTerm* term) const {
  GET_RELATED_TERMS_IN_ARCS(outputToClockArcs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedInputs_(SNLInstTerm* iterm) const {
  GET_RELATED_INSTTERMS_IN_ARCS(clockToInputArcs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedOutputs_(SNLInstTerm* iterm) const {
  GET_RELATED_INSTTERMS_IN_ARCS(clockToOutputArcs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getInputRelatedClocks_(SNLInstTerm* iterm) const {
  GET_RELATED_INSTTERMS_IN_ARCS(inputToClockArcs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getOutputRelatedClocks_(SNLInstTerm* iterm) const {
  GET_RELATED_INSTTERMS_IN_ARCS(outputToClockArcs_)
}

void SNLDesignModeling::addCombinatorialArcs(
  const BitTerms& inputs, const BitTerms& outputs) {
  auto design = verifyInputs(inputs, "inputs", outputs, "outputs", "addCombinatorialArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto input: inputs) {
    for (auto output: outputs) {
      modeling->addCombinatorialArc_(input, output);
    }
  }
}

void SNLDesignModeling::addCombinatorialArcs(
  const std::string& parameterValue,
  const BitTerms& inputs, const BitTerms& outputs) {
  auto design = verifyInputs(inputs, "inputs", outputs, "outputs", "addCombinatorialArcs");
  auto property = getOrCreateProperty(design, Type::PARAMETERIZED);
  auto modeling = property->getModeling();
  for (auto input: inputs) {
    for (auto output: outputs) {
      modeling->addCombinatorialArc_(input, output, parameterValue);
    }
  }
}

void SNLDesignModeling::addInputsToClockArcs(const BitTerms& inputs, SNLBitTerm* clock) {
  auto design = verifyInputs(inputs, "inputs", {clock}, "clock", "addInputsToClockArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto input: inputs) {
    modeling->addInputToClockArc_(input, clock);
  }
}

void SNLDesignModeling::addClockToOutputsArcs(SNLBitTerm* clock, const BitTerms& outputs) {
  auto design = verifyInputs({clock}, "clock", outputs, "outputs", "addClockToOutputsArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto output: outputs) {
    modeling->addClockToOutputArc_(clock, output);
  }
}

void SNLDesignModeling::setParameter(SNLDesign* design, const std::string& name, const std::string& defaultValue) {
  auto parameter = design->getParameter(NLName(name));
  if (not parameter) {
    std::ostringstream reason;
    reason << "Parameter " << name << " is unknown in " << design->getName().getString();
    throw NLException(reason.str());
  }
  auto property = getOrCreateProperty(design, Type::PARAMETERIZED);
  auto modeling = property->getModeling();
  modeling->parameter_ = std::make_pair(name, defaultValue);
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs(SNLBitTerm* term) {
  GET_RELATED_OBJECTS(SNLBitTerm, term, getDesign(), getCombinatorialOutputs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialOutputs(SNLInstTerm* iterm) {
  GET_RELATED_OBJECTS(SNLInstTerm, iterm, getInstance()->getModel(), getCombinatorialOutputs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs(SNLBitTerm* term) {
  GET_RELATED_OBJECTS(SNLBitTerm, term, getDesign(), getCombinatorialInputs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialInputs(SNLInstTerm* iterm) {
  GET_RELATED_OBJECTS(SNLInstTerm, iterm, getInstance()->getModel(), getCombinatorialInputs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedInputs(SNLBitTerm* clock) {
  GET_RELATED_OBJECTS(SNLBitTerm, clock, getDesign(), getClockRelatedInputs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedOutputs(SNLBitTerm* clock) {
  GET_RELATED_OBJECTS(SNLBitTerm, clock, getDesign(), getClockRelatedOutputs_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getInputRelatedClocks(SNLBitTerm* input) {
  GET_RELATED_OBJECTS(SNLBitTerm, input, getDesign(), getInputRelatedClocks_)
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getOutputRelatedClocks(SNLBitTerm* output) {
  GET_RELATED_OBJECTS(SNLBitTerm, output, getDesign(), getOutputRelatedClocks_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedInputs(SNLInstTerm* clock) {
  GET_RELATED_OBJECTS(SNLInstTerm, clock, getInstance()->getModel(), getClockRelatedInputs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedOutputs(SNLInstTerm* clock) {
  GET_RELATED_OBJECTS(SNLInstTerm, clock, getInstance()->getModel(), getClockRelatedOutputs_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getInputRelatedClocks(SNLInstTerm* input) {
  GET_RELATED_OBJECTS(SNLInstTerm, input, getInstance()->getModel(), getInputRelatedClocks_)
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getOutputRelatedClocks(SNLInstTerm* output) {
  GET_RELATED_OBJECTS(SNLInstTerm, output, getInstance()->getModel(), getOutputRelatedClocks_)
}

}} // namespace NL // namespace naja
