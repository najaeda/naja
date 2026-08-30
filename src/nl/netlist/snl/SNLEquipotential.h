// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>
#include "SNLNet.h"
#include "SNLBitTerm.h"
#include "SNLOccurrence.h"

namespace naja::NL {

class SNLNetComponent;

class SNLEquipotential {
  public:
    enum class Mode {
      Standard,
      /// Cross Assign instances and omit their inst terms from the result.
      TraverseAssigns
    };

    using InstTermOccurrences = std::set<SNLOccurrence>;
    using Terms = std::set<SNLBitTerm*, SNLDesignObject::PointerLess>;

    SNLEquipotential()=default;
    SNLEquipotential(const SNLEquipotential&)=default;
    SNLEquipotential(SNLNetComponent* netComponent);
    SNLEquipotential(SNLNetComponent* netComponent, Mode mode);
    SNLEquipotential(const SNLOccurrence& netComponentOccurrence);
    SNLEquipotential(
      const SNLOccurrence& netComponentOccurrence,
      Mode mode);

    const Terms& getTermsSet() const { return terms_; }
    const InstTermOccurrences& getInstTermOccurrencesSet() const { return instTermOccurrences_; }
    NajaCollection<SNLBitTerm*> getTerms() const;
    NajaCollection<SNLOccurrence> getInstTermOccurrences() const;
    SNLNet::Type getType() const { return type_; }
    bool isConst0() const;
    bool isConst1() const;
    bool isConstX() const;
    bool isConstZ() const;
    std::string getString() const;
    //Comparators
    bool operator==(const SNLEquipotential& other) const {
      return instTermOccurrences_ == other.instTermOccurrences_ and terms_ == other.terms_;
    }
    bool operator!=(const SNLEquipotential& other) const {
      return not operator==(other);
    }
    bool operator<(const SNLEquipotential& other) const {
      return instTermOccurrences_ < other.instTermOccurrences_ or
        (instTermOccurrences_ == other.instTermOccurrences_ and terms_ < other.terms_);
    }
    bool operator>(const SNLEquipotential& other) const {
      return instTermOccurrences_ > other.instTermOccurrences_ or
        (instTermOccurrences_ == other.instTermOccurrences_ and terms_ > other.terms_);
    }
    bool operator<=(const SNLEquipotential& other) const {
      return not operator>(other);
    }
    bool operator>=(const SNLEquipotential& other) const {
      return not operator<(other);
    }
  private:
    InstTermOccurrences instTermOccurrences_  {};
    Terms               terms_                {};
    SNLNet::Type        type_                 {SNLNet::Type::Standard};
};

}  // namespace naja::NL
