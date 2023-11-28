// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INST_TERM_OCCURRENCE_H_
#define __SNL_INST_TERM_OCCURRENCE_H_

#include "SNLNetComponentOccurrence.h"

namespace naja { namespace SNL {

class SNLInstTerm;

/**
 * \brief SNLInstTermOccurrence is the specialization class of SNLOccurrence for SNLInstTerm.
 */
class SNLInstTermOccurrence: public SNLNetComponentOccurrence {
  public:
    using super = SNLNetComponentOccurrence;
    
    /// \brief SNLInstTermOccurrence empty constructor.
    SNLInstTermOccurrence() = default;
    /// \brief SNLInstTermOccurrence constructor with empty path.
    SNLInstTermOccurrence(SNLInstTerm* instTerm);
    /**
     * \brief SNLInstTermOccurrence constructor.
     * \param path SNLPath to occurrence.
     * \param instTerm referenced SNLInstTerm.
     */
    SNLInstTermOccurrence(const SNLPath& path, SNLInstTerm* instTerm);

    /// \return the referenced SNLInstTerm.
    SNLInstTerm* getInstTerm() const;
};

}} // namespace SNL // namespace naja

#endif // __SNL_INST_TERM_OCCURRENCE_H_