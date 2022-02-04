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

#ifndef __SNL_SCALAR_NET_H_
#define __SNL_SCALAR_NET_H_

#include "SNLBitNet.h"
#include "SNLName.h"

namespace SNL {

class SNLScalarNet final: public SNLBitNet {
  public:
    friend class SNLDesign;
    using super = SNLBitNet;

    static SNLScalarNet* create(SNLDesign* design, const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }

    SNLID getSNLID() const override;
    SNLID::DesignObjectID getID() const override { return id_; }

    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }

    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;
  private:
    SNLScalarNet(SNLDesign* design, const SNLName& name);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
};

}

#endif /* __SNL_SCALAR_NET_H_ */ 
