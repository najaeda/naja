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

#ifndef __SNL_NET_H_
#define __SNL_NET_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "SNLDesignObject.h"

namespace SNL {

class SNLNet: public SNLDesignObject {
  public:
    friend class SNLDesign;
    using super = SNLDesignObject;

    virtual SNLID::DesignObjectID getID() const = 0;
    virtual SNLName getName() const = 0;

  protected:
    SNLNet() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    //following used in BusNet and ScalarNet
    virtual void setID(SNLID::DesignObjectID id) = 0;
    boost::intrusive::set_member_hook<> designNetsHook_ {};

    virtual void destroyFromDesign() = 0;
};

}

#endif /* __SNL_NET_H_ */ 
