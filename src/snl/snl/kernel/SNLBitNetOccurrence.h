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

    SNLBitNetOccurrence() = default;
    SNLBitNetOccurrence(SNLBitNet* net);
    SNLBitNetOccurrence(const SNLPath& path, SNLBitNet* net);

    SNLBitNet* getNet() const;
};

}} // namespace SNL // namespace naja

#endif // __SNL_OCCURRENCE_H_
