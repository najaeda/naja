// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
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

    /// \brief SNLBitTermOccurrence empty constructor.
    SNLBitTermOccurrence() = default;
    /**
     * \brief SNLBitTermOccurrence constructor with an empty path.
     * \param term referenced SNLBitTerm.
     */
    SNLBitTermOccurrence(SNLBitTerm* term);
    /**
     * \brief SNLBitTermOccurrence constructor.
     * \param path SNLPath to the referenced SNLBitNet. 
     * \param term referenced SNLBitTerm.
     */
    SNLBitTermOccurrence(const SNLPath& path, SNLBitTerm* term);

    /// \return referenced SNLBitTerm.
    SNLBitTerm* getTerm() const;
};

}} // namespace SNL // namespace naja

#endif // __SNL_BIT_TERM_OCCURRENCE_H_