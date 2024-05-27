// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignTruthTable.h"

#include <sstream>

#include "NajaDumpableProperty.h"
#include "SNLDesign.h"
#include "SNLException.h"

namespace {

class SNLDesignTruthTableProperty: public naja::NajaDumpableProperty {
  public:
    using Inherit = naja::NajaDumpableProperty;
    static const inline std::string Name = "SNLDesignTruthTableProperty";
    static SNLDesignTruthTableProperty* create(
      naja::SNL::SNLDesign* design,
      const naja::SNL::SNLTruthTable& truthTable) {
      preCreate(design, Name);
      SNLDesignTruthTableProperty* property = new SNLDesignTruthTableProperty(truthTable);
      property->postCreate(design);
      return property;
    }
    static void preCreate(naja::SNL::SNLDesign* design, const std::string& name) {
      Inherit::preCreate(design, name);
      if (not (design->isPrimitive())) {
        std::ostringstream reason;
        reason << "Impossible to add Truth Table on a non primitive design <"
          << design->getName().getString() << ">";
        throw naja::SNL::SNLException(reason.str());
      }
    }
    SNLDesignTruthTableProperty(const naja::SNL::SNLTruthTable& truthTable)
      : Inherit(Name), truthTable_(truthTable) {}
    std::string getName() const override {
      return Name;
    }
    //LCOV_EXCL_START
    std::string getString() const override {
      return Name;
    }
    //LCOV_EXCL_STOP
    naja::SNL::SNLTruthTable getTruthTable() const {
      return truthTable_;
    }
  private:
    naja::SNL::SNLTruthTable truthTable_ {};
};

SNLDesignTruthTableProperty* getProperty(const naja::SNL::SNLDesign* design) {
  auto property =
    static_cast<SNLDesignTruthTableProperty*>(design->getProperty(SNLDesignTruthTableProperty::Name));
  if (property) {
    return property;
  }
  return nullptr;
}

SNLDesignTruthTableProperty* createProperty(
  naja::SNL::SNLDesign* design,
  const naja::SNL::SNLTruthTable& truthTable) {
  auto property = getProperty(design);
  if (property) {
    throw naja::SNL::SNLException("Design already has a Truth Table");
  } 
  return SNLDesignTruthTableProperty::create(design, truthTable);
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
    return property->getTruthTable();
  }
  return SNLTruthTable();
}

}} // namespace SNL // namespace naja