// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __SNL_EQUIPOTENTIAL_H_
#define __SNL_EQUIPOTENTIAL_H_

#include <set>
#include "SNLBitTerm.h"
#include "SNLInstTermOccurrence.h"

namespace naja { namespace NL {

class SNLNetComponent;
class SNLNetComponentOccurrence;

class SNLEquipotential {
  public:
    //SNLEquipotential()=delete;
    //SNLEquipotential(const SNLEquipotential&)=delete;
    using InstTermOccurrences = std::set<SNLInstTermOccurrence>;
    using Terms = std::set<SNLBitTerm*, SNLDesignObject::PointerLess>;

    SNLEquipotential(SNLNetComponent* netComponent);
    SNLEquipotential(const SNLNetComponentOccurrence& netComponentOccurrence);

    const Terms& getTermsSet() const { return terms_; }
    const InstTermOccurrences& getInstTermOccurrencesSet() const { return instTermOccurrences_; }
    NajaCollection<SNLBitTerm*> getTerms() const;
    NajaCollection<SNLInstTermOccurrence> getInstTermOccurrences() const;
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
};

}} // namespace NL // namespace naja

#endif // __SNL_EQUIPOTENTIAL_H_
