// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BIT_NET_OCCURRENCE_H_
#define __SNL_BIT_NET_OCCURRENCE_H_

#include "SNLOccurrence.h"

namespace naja { namespace SNL {

class SNLBitNet;

class SNLBitNetOccurrence: public SNLOccurrence {
  public:
    using super = SNLOccurrence;

    /// \brief SNLBitNetOccurrence empty constructor.
    SNLBitNetOccurrence() = default;
    /**
     * \brief SNLBitNetOccurrence constructor with an empty path.
     * \param net referenced SNLBitNet.
     */
    SNLBitNetOccurrence(SNLBitNet* net);
    /**
     * \brief SNLBitNetOccurrence constructor.
     * \param path SNLPath to the referenced SNLBitNet. 
     * \param net referenced SNLBitNet.
     */
    SNLBitNetOccurrence(const SNLPath& path, SNLBitNet* net);

    /// \return referenced SNLBitNet. 
    SNLBitNet* getNet() const;
};

}} // namespace SNL // namespace naja

#endif // __SNL_OCCURRENCE_H_
