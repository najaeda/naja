// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_NET_COMPONENT_OCCURRENCE_H_
#define __SNL_NET_COMPONENT_OCCURRENCE_H_

#include "SNLOccurrence.h"

namespace naja { namespace SNL {

class SNLNetComponent;
class SNLBitNet;
class SNLBitNetOccurrence;

class SNLNetComponentOccurrence: public SNLOccurrence {
  public:
    using super = SNLOccurrence;

    SNLNetComponentOccurrence() = default;
    SNLNetComponentOccurrence(SNLNetComponent* component);
    SNLNetComponentOccurrence(const SNLPath& path, SNLNetComponent* component);

    SNLNetComponent* getComponent() const;
    SNLBitNet* getNet() const;
    SNLBitNetOccurrence getNetOccurrence() const;
};

}} // namespace SNL // namespace naja

#endif // __SNL_NET_COMPONENT_OCCURRENCE_H_