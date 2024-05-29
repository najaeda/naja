// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignTruthTable.h"

#include <sstream>

#include "NajaDumpableProperty.h"
#include "SNLDesign.h"
#include "SNLException.h"

namespace {
    
static const std::string SNLDesignTruthTablePropertyName = "SNLDesignTruthTableProperty";

naja::NajaDumpableProperty* getProperty(const naja::SNL::SNLDesign* design) {
  auto property =
    static_cast<naja::NajaDumpableProperty*>(design->getProperty(SNLDesignTruthTablePropertyName));
  if (property) {
    return property;
  }
  return nullptr;
}

void createProperty(
  naja::SNL::SNLDesign* design,
  const naja::SNL::SNLTruthTable& truthTable) {
  auto property = getProperty(design);
  if (property) {
    throw naja::SNL::SNLException("Design already has a Truth Table");
  }
  property = naja::NajaDumpableProperty::create(design, SNLDesignTruthTablePropertyName);
  property->addUInt64Value(truthTable.size());
  property->addUInt64Value(truthTable.bits());
}

} // namespace

namespace naja { namespace SNL {

void SNLDesignTruthTable::setTruthTable(SNLDesign* design, const SNLTruthTable& truthTable) {
  if (not design->isPrimitive()) {
    throw SNLException("Cannot add truth table on non-primitive design");
  }
  //verify that the truth table size == nb of inputs
  auto inputFilter = [](const SNLTerm* term) { return term->getDirection() == SNLTerm::Direction::Input; };
  auto inputs = design->getTerms().getSubCollection(inputFilter);
  if (inputs.size() not_eq truthTable.size()) {
    std::ostringstream reason;
    reason << "Truth table size <" << truthTable.size() << "> is different from inputs size <"
      << inputs.size() << "> in design <" << design->getName().getString() << ">";
    throw SNLException(reason.str());
  }
  auto outputFilter = [](const SNLTerm* term) { return term->getDirection() == SNLTerm::Direction::Output; };
  auto outputs = design->getTerms().getSubCollection(outputFilter);
  if (outputs.size() not_eq 1) {
    std::ostringstream reason;
    reason << "cannot add truth table on Design <" << design->getName().getString() << "> that has <" << outputs.size() << "> outputs";
    throw SNLException(reason.str());
  }
  createProperty(design, truthTable);
}

SNLTruthTable SNLDesignTruthTable::getTruthTable(const SNLDesign* design) {
  auto property = getProperty(design); 
  if (property) {
    return naja::SNL::SNLTruthTable(
      (uint32_t)property->getUInt64Value(0),
      property->getUInt64Value(1)
    );
  }
  return naja::SNL::SNLTruthTable();
}

bool SNLDesignTruthTable::isConst0(const SNLDesign* design) {
  auto truthTable = getTruthTable(design);
  if (truthTable.isInitialized()) {
    return truthTable == SNLTruthTable(0, 0);
  }
  return false;
}

bool SNLDesignTruthTable::isConst1(const SNLDesign* design) {
  auto truthTable = getTruthTable(design);
  if (truthTable.isInitialized()) {
    return truthTable == SNLTruthTable(0, 1);
  }
  return false;
}

bool SNLDesignTruthTable::isInv(const SNLDesign* design) {
  auto truthTable = getTruthTable(design);
  if (truthTable.isInitialized()) {
    return truthTable == SNLTruthTable(1, 0b01);
  }
  return false;
}

bool SNLDesignTruthTable::isBuf(const SNLDesign* design) {
  auto truthTable = getTruthTable(design);
  if (truthTable.isInitialized()) {
    return truthTable == SNLTruthTable(1, 0b10);
  }
  return false;
}

}} // namespace SNL // namespace naja