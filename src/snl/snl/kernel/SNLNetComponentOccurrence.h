// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_NET_COMPONENT_OCCURRENCE_H_
#define __SNL_NET_COMPONENT_OCCURRENCE_H_

#include "SNLOccurrence.h"

namespace naja { namespace SNL {

class SNLNetComponent;
class SNLBitNet;
class SNLBitNetOccurrence;

/**
 * \brief SNLNetComponentOccurrence is the specialization class of SNLOccurrence for SNLNetComponent.
*/
class SNLNetComponentOccurrence: public SNLOccurrence {
  public:
    using super = SNLOccurrence;

    SNLNetComponentOccurrence() = default;
    SNLNetComponentOccurrence(SNLNetComponent* component);
    SNLNetComponentOccurrence(const SNLPath& path, SNLNetComponent* component);

    SNLNetComponent* getComponent() const;
    /// \return the SNLBitNet connected to the referenced SNLNetComponent.
    SNLBitNet* getNet() const;
    /// \return the SNLBitNetOccurrence of the connected SNLBitNet.
    SNLBitNetOccurrence getNetOccurrence() const;
};

}} // namespace SNL // namespace naja

#endif // __SNL_NET_COMPONENT_OCCURRENCE_H_