// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignModeling.h"

#include <sstream>

#include "NajaPrivateProperty.h"
#include "NajaDumpableProperty.h"
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

static const std::string SNLDesignTruthTablePropertyName =
    "SNLDesignTruthTableProperty";

naja::NajaDumpableProperty* getTruthTableProperty(const naja::NL::SNLDesign* design) {
  auto property = static_cast<naja::NajaDumpableProperty*>(
      design->getProperty(SNLDesignTruthTablePropertyName));
  return property;
}

void createTruthTableProperty(naja::NL::SNLDesign* design,
                    const naja::NL::SNLTruthTable& truthTable) {
  // LCOV_EXCL_START
  if (getTruthTableProperty(design)) {
    throw naja::NL::NLException("Design already has a Truth Table");
  }
  // LCOV_EXCL_STOP
  auto property = naja::NajaDumpableProperty::create(
      design, SNLDesignTruthTablePropertyName);
  property->addUInt64Value(truthTable.size());
  for (auto mask : truthTable.bits().getChunks()) {
    property->addUInt64Value(mask);
  }
}

void createTruthTableProperty(naja::NL::SNLDesign* design,
                    const std::vector<naja::NL::SNLTruthTable>& truthTables) {
  if (truthTables.empty()) {
    throw naja::NL::NLException("Cannot set empty truth table");
  }
  // LCOV_EXCL_START
  if (getTruthTableProperty(design)) {
    throw naja::NL::NLException("Design already has a Truth Table");
  }
  // LCOV_EXCL_STOP
  auto property = naja::NajaDumpableProperty::create(
      design, SNLDesignTruthTablePropertyName);
  for (const auto& truthTable : truthTables) {
    property->addUInt64Value(truthTable.size());
    for (auto mask : truthTable.bits().getChunks()) {
      property->addUInt64Value(mask);
    }
  }
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getCombinatorialTermsFromTruthTable(naja::NL::SNLBitTerm* term) {
  size_t tableCount = naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign());
  if (tableCount > 0) {
    //make the assumption that if there is a tt,
    //then all opposite terms are combi related.
    if (term->getDirection() == naja::NL::SNLTerm::Direction::Input) {
      return term->getDesign()->getBitTerms().getSubCollection(
        [](naja::NL::SNLBitTerm* t) {
          return t->getDirection() == naja::NL::SNLTerm::Direction::Output;
        }
      );
    } else if (term->getDirection() == naja::NL::SNLTerm::Direction::Output) {
      //return all inputs
      return term->getDesign()->getBitTerms().getSubCollection(
        [](naja::NL::SNLBitTerm* t) {
          return t->getDirection() == naja::NL::SNLTerm::Direction::Input;
        }
      );
    }
  }
  return {};
}

}  // namespace
  
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
  auto property = getProperty(term->getDesign());
  if (property) {
    GET_RELATED_OBJECTS(SNLBitTerm, term, getDesign(), getCombinatorialOutputs_)
  } else {
    return getCombinatorialTermsFromTruthTable(term);
  }
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialOutputs(SNLInstTerm* iterm) {
  auto property = getProperty(iterm->getInstance()->getModel());
  if (property) {
    GET_RELATED_OBJECTS(SNLInstTerm, iterm, getInstance()->getModel(), getCombinatorialOutputs_)
  } else {
    return getCombinatorialTermsFromTruthTable(iterm->getBitTerm()).getTransformerCollection<SNLInstTerm*>(
      [=](const SNLBitTerm* term) { return iterm->getInstance()->getInstTerm(term); });
  }
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs(SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    GET_RELATED_OBJECTS(SNLBitTerm, term, getDesign(), getCombinatorialInputs_)
  } else {
    return getCombinatorialTermsFromTruthTable(term);
  }
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialInputs(SNLInstTerm* iterm) {
  auto property = getProperty(iterm->getInstance()->getModel());
  if (property) {
    GET_RELATED_OBJECTS(SNLInstTerm, iterm, getInstance()->getModel(), getCombinatorialInputs_)
  } else {
    return getCombinatorialTermsFromTruthTable(iterm->getBitTerm()).getTransformerCollection<SNLInstTerm*>(
      [=](const SNLBitTerm* term) { return iterm->getInstance()->getInstTerm(term); });
  }
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

void SNLDesignModeling::setTruthTable(SNLDesign* design,
                                        const SNLTruthTable& truthTable) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add truth table on non-primitive design");
  }
  // Check no truth table already exists
  if (getTruthTableProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }
  auto outputs = design->getTerms().getSubCollection(
      [](const SNLTerm* t) {
        return t->getDirection() == SNLTerm::Direction::Output;
      });
  if (outputs.size() != 1) {
    std::ostringstream reason;
    reason << "cannot add truth table on Design <"
           << design->getName().getString() << "> that has <"
           << outputs.size() << "> outputs";
    throw NLException(reason.str());
  }
  createTruthTableProperty(design, truthTable);
}

void SNLDesignModeling::setTruthTables(
    SNLDesign* design,
    const std::vector<SNLTruthTable>& truthTables) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add truth table on non-primitive design");
  }
  // Check no truth table already exists
  if (getTruthTableProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }
  auto outputs = design->getTerms().getSubCollection(
      [](const SNLTerm* t) {
        return t->getDirection() == SNLTerm::Direction::Output;
      });
  if (outputs.size() != truthTables.size()) {
    std::ostringstream reason;
    reason << "cannot add truth tables on Design <"
           << design->getName().getString() << "> that has <"
           << outputs.size() << "> outputs, but provided <"
           << truthTables.size() << "> truth tables";
    throw NLException(reason.str());
  }
  createTruthTableProperty(design, truthTables);
}

size_t SNLDesignModeling::getTruthTableCount(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  size_t tableIdx = 0;
  if (property) {
    // scan through each stored table until we reach outputID
    size_t valIdx   = 0;
    size_t total    = property->getValues().size();

    while (valIdx < total) {
      uint32_t nInputs = static_cast<uint32_t>(
          property->getUInt64Value(valIdx));
      size_t   nBits   = 1u << nInputs;
      size_t   nChunks = nBits / 64 + ((nBits % 64) > 0 ? 1 : 0);
      valIdx += 1 + nChunks;
      // LCOV_EXCL_START
      if (valIdx >= total + 1 /*because this loop will take you to the next table, therefore the + 1*/) {
        std::ostringstream reason;
        // create a string by concating all values
        std::string result = "";
        for (size_t i = 0; i < total; ++i) {
          result += std::to_string(property->getUInt64Value(i)) + " ";
        }
        reason << "Maldformed truth table for design <"
               << design->getName().getString() << ">" << " " << result << "\n" 
               << "With valIdx " << valIdx << " and total " << total;
        throw NLException(reason.str());
      }
      // LCOV_EXCL_STOP
      ++tableIdx;
    }
  }
  return tableIdx;
}

SNLTruthTable SNLDesignModeling::getTruthTable(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  if (property) {
    // total number of mask‐values trailing the first “size” entry
    size_t tableSize = property->getValues().size() - 1;

    // how many chunks *should* we have, given the stored input‐count?
    uint64_t declaredInputs = property->getUInt64Value(0);
    uint64_t num_bits = 1u << declaredInputs;
    size_t   expectedChunks =
        (declaredInputs == 0 && tableSize == 1)
            ? 1
            : (num_bits / 64 + ((num_bits % 64) > 0 ? 1 : 0));
    if (expectedChunks != tableSize) {
      std::ostringstream reason;
      reason << "Truth table size " << tableSize
             << " does not match number of chunks " << expectedChunks << " which suggests per output functionality";
      throw NLException(reason.str());
    }

    // multi‐chunk (i.e. >64‐bit) table?
    if (property->getValues().size() > 2) {
      uint32_t numInputs = static_cast<uint32_t>(declaredInputs);
      uint32_t nBits     = 1u << numInputs;
      if (nBits <= 64) {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "Truth table size " << nBits
               << " is not larger than 64 bits";
        throw NLException(reason.str());
        // LCOV_EXCL_STOP
      }

      std::vector<bool> bits(nBits, false);
      size_t            nChunks = (nBits + 63) / 64;

      for (size_t c = 0; c < nChunks; ++c) {
        uint64_t mask = property->getUInt64Value(c + 1);
        for (size_t b = 0; b < 64; ++b) {
          size_t pos = c * 64 + b;
          if (pos >= nBits) break;
          if ((mask >> b) & 1) bits[pos] = true;
        }
      }

      return SNLTruthTable(numInputs, bits);
    }
    // single‐chunk
    if (declaredInputs <= 6) {
      return SNLTruthTable(static_cast<uint32_t>(declaredInputs),
                           property->getUInt64Value(1));
    } else {
      // LCOV_EXCL_START
      std::ostringstream reason;
      reason << "Truth table size " << declaredInputs
             << " is larger than 64 bits";
      throw NLException(reason.str());
      // LCOV_EXCL_STOP
    }
  }
  return SNLTruthTable();
}

SNLTruthTable SNLDesignModeling::getTruthTable(
    const SNLDesign* design,
    NLID::DesignObjectID termID) {
  auto property = getTruthTableProperty(design);
  std::map<NLID::DesignObjectID, NLID::DesignObjectID> termID2outputID;
  NLID::DesignObjectID outputIndex = 0;
  for (const auto& term : design->getTerms()) {
    if (term->getDirection() == SNLTerm::Direction::Output) {
      termID2outputID[term->getID()] = outputIndex;
      ++outputIndex;
    }
  }
  if (termID2outputID.find(termID) == termID2outputID.end()) {
    std::ostringstream reason;
    reason << "Term ID " << termID
           << " not found in design <" << design->getName().getString() << ">";
    throw NLException(reason.str());
  }
  NLID::DesignObjectID outputID = termID2outputID[termID];
  if (property) {
    // scan through each stored table until we reach outputID
    if (getTruthTableCount(design) == 1) {
      // if there is only one table, then it is the same for all outputs
      return getTruthTable(design);
    }
    size_t tableIdx = 0;
    size_t valIdx   = 0;
    size_t total    = property->getValues().size();

    while (true) {
      // LCOV_EXCL_START
      if (valIdx >= total) {
        std::ostringstream reason;
        reason << "Output ID " << outputID
               << " is out of range for design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      // LCOV_EXCL_STOP
      if (tableIdx >= outputID) {
        break;
      }
      uint32_t nInputs = static_cast<uint32_t>(
          property->getUInt64Value(valIdx));
      size_t   nBits   = 1u << nInputs;
      size_t   nChunks = nBits / 64 + ((nBits % 64) > 0 ? 1 : 0);
      valIdx += 1 + nChunks;
      ++tableIdx;
    }

    uint64_t declaredInputs = property->getUInt64Value(valIdx);
    // single‐chunk fast‐path?
    if (declaredInputs <= 6) {
      return SNLTruthTable(static_cast<uint32_t>(declaredInputs),
                           property->getUInt64Value(valIdx + 1));
    }

    // else multi‐chunk
    uint32_t numInputs = static_cast<uint32_t>(declaredInputs);
    uint32_t nBits     = 1u << numInputs;
    size_t   nChunks   = nBits / 64 + ((nBits % 64) > 0 ? 1 : 0);

    std::vector<bool> bits(nBits, false);
    for (size_t c = 0; c < nChunks; ++c) {
      uint64_t mask = property->getUInt64Value(valIdx + 1 + c);
      for (size_t b = 0; b < 64; ++b) {
        size_t pos = c * 64 + b;
        if (pos >= nBits) break;
        if ((mask >> b) & 1) bits[pos] = true;
      }
    }
    return SNLTruthTable(numInputs, bits);
  }
  return SNLTruthTable();
}

bool SNLDesignModeling::hasModeling(const SNLDesign* design) {
  auto property = getProperty(design);
  if (property) {
    return true;
  } else {
    return getTruthTableProperty(design);
  }
}

bool SNLDesignModeling::isSequential(const SNLDesign* design) {
  auto property = getProperty(design);
  if (property) {
    auto modeling = property->getModeling();
    const auto arcs = modeling->getTimingArcs();
    return not arcs->inputToClockArcs_.empty() or not arcs->clockToInputArcs_.empty();
  }
  return false;
}

bool SNLDesignModeling::isConst0(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         truthTable == SNLTruthTable::Logic0();
}

bool SNLDesignModeling::isConst1(const SNLDesign* design) {
   auto property = getTruthTableProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         truthTable == SNLTruthTable::Logic1();
}

bool SNLDesignModeling::isConst(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         (truthTable == SNLTruthTable::Logic0() ||
          truthTable == SNLTruthTable::Logic1());
}

bool SNLDesignModeling::isInv(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         truthTable == SNLTruthTable::Inv();
}

bool SNLDesignModeling::isBuf(const SNLDesign* design) {
   auto property = getTruthTableProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         truthTable == SNLTruthTable::Buf();
}

}} // namespace NL // namespace naja
