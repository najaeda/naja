// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>
#include "SNLNet.h"
#include "SNLBitTerm.h"
#include "SNLOccurrence.h"

namespace naja { namespace NL {

class SNLNetComponent;

class SNLEquipotential {
  public:
    using InstTermOccurrences = std::set<SNLOccurrence>;
    using Terms = std::set<SNLBitTerm*, SNLDesignObject::PointerLess>;

    SNLEquipotential()=default;
    SNLEquipotential(const SNLEquipotential&)=default;
    SNLEquipotential(SNLNetComponent* netComponent);
    SNLEquipotential(const SNLOccurrence& netComponentOccurrence);

    const Terms& getTermsSet() const { return terms_; }
    const InstTermOccurrences& getInstTermOccurrencesSet() const { return instTermOccurrences_; }
    NajaCollection<SNLBitTerm*> getTerms() const;
    NajaCollection<SNLOccurrence> getInstTermOccurrences() const;
    SNLNet::Type getType() const { return type_; }
    bool isConst0() const;
    bool isConst1() const;
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

}} // namespace NL // namespace naja