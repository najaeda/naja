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
  //verify that the truth table size == nb of inputs
  auto filter = [](const SNLTerm* term) { return term->getDirection() == SNLTerm::Direction::Input; };
  auto inputs = design->getTerms().getSubCollection(filter);
  if (inputs.size() not_eq truthTable.size()) {
    std::ostringstream reason;
    reason << "Truth table size <" << truthTable.size() << "> is different from inputs size <"
      << inputs.size() << "> in design <" << design->getName().getString() << ">";
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

}} // namespace SNL // namespace naja