// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __SNL_EQUIPOTENTIAL_H_
#define __SNL_EQUIPOTENTIAL_H_

#include <set>
#include "SNLBitTerm.h"

namespace naja { namespace SNL {

class SNLNetComponent;
class SNLNetComponentOccurrence;
class SNLInstTermOccurrence;

class SNLEquipotential {
  public:
    SNLEquipotential()=delete;
    SNLEquipotential(const SNLEquipotential&)=delete;
    using InstTermOccurrences = std::set<SNLInstTermOccurrence>;
    using Terms = std::set<SNLBitTerm*, SNLDesignObject::PointerLess>;

    SNLEquipotential(SNLNetComponent* netComponent);
    SNLEquipotential(const SNLNetComponentOccurrence& netComponentOccurrence);

    Terms getTerms() const { return terms_; }
    InstTermOccurrences getInstTermOccurrences() const { return instTermOccurrences_; }
  private:
    InstTermOccurrences instTermOccurrences_  {};
    Terms               terms_                {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_EQUIPOTENTIAL_H_
