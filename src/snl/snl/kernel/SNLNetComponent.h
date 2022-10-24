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

#ifndef __SNL_NET_COMPONENT_H_
#define __SNL_NET_COMPONENT_H_

#include <boost/intrusive/set.hpp>

#include "SNLDesignObject.h"

namespace naja { namespace SNL {

class SNLBitNet;

class SNLNetComponent: public SNLDesignObject {
  public:
    friend class SNLBitNet;
    using super = SNLDesignObject;

    virtual SNLBitNet* getNet() const =0;
    virtual void setNet(SNLBitNet* net) =0;

  protected:
    SNLNetComponent() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    boost::intrusive::set_member_hook<> netComponentsHook_  {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_NET_COMPONENT_H_