/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __SNL_BIT_NET_OCCURRENCE_H_
#define __SNL_BIT_NET_OCCURRENCE_H_

#include "SNLOccurrence.h"

namespace naja { namespace SNL {

class SNLBitNet;

class SNLBitNetOccurrence: public SNLOccurrence {
  public:
    using super = SNLOccurrence;

    SNLBitNetOccurrence()=default;
    SNLBitNetOccurrence(SNLBitNet* net);
    SNLBitNetOccurrence(const SNLPath& path, SNLBitNet* net);

    SNLBitNet* getNet() const { return net_; }
  private:
    SNLBitNet*  net_  {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_OCCURRENCE_H_
