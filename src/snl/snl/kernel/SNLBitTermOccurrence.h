// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BIT_TERM_OCCURRENCE_H_
#define __SNL_BIT_TERM_OCCURRENCE_H_

#include "SNLNetComponentOccurrence.h"

namespace naja { namespace SNL {

class SNLBitTerm;

class SNLBitTermOccurrence: public SNLNetComponentOccurrence {
  public:
    using super = SNLNetComponentOccurrence;

    SNLBitTermOccurrence() = default;
    SNLBitTermOccurrence(SNLBitTerm* term);
    SNLBitTermOccurrence(const SNLPath& path, SNLBitTerm* term);

    SNLBitTerm* getTerm() const;
};

}} // namespace SNL // namespace naja

#endif // __SNL_BIT_TERM_OCCURRENCE_H_