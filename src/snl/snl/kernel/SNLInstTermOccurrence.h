// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INST_TERM_OCCURRENCE_H_
#define __SNL_INST_TERM_OCCURRENCE_H_

#include "SNLNetComponentOccurrence.h"

namespace naja { namespace SNL {

class SNLInstTerm;

class SNLInstTermOccurrence: public SNLNetComponentOccurrence {
  public:
    using super = SNLNetComponentOccurrence;
    
    SNLInstTermOccurrence() = default;
    SNLInstTermOccurrence(SNLInstTerm* instTerm);
    SNLInstTermOccurrence(const SNLPath& path, SNLInstTerm* instTerm);

    SNLInstTerm* getInstTerm() const;
};

}} // namespace SNL // namespace naja

#endif // __SNL_INST_TERM_OCCURRENCE_H_