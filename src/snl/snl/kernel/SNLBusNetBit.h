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

#ifndef __SNL_BUS_NET_BIT_H_
#define __SNL_BUS_NET_BIT_H_

#include "SNLBitNet.h"

namespace naja { namespace SNL {

class SNLBusNet;

class SNLBusNetBit final: public SNLBitNet {
  public:
    friend class SNLBusNet;
    using super = SNLBitNet;

    SNLDesign* getDesign() const override;

    //\remark returns the owner SNLBusNet ID
    SNLID::DesignObjectID getID() const override;
    SNLID getSNLID() const override;
    SNLBusNet* getBus() const { return bus_; }
    SNLID::Bit getBit() const { return bit_; }

    const char* getTypeName() const override;
    SNLName getName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool isAnonymous() const override;

    void destroy() override;
  private:
    static SNLBusNetBit* create(SNLBusNet* bus, SNLID::Bit bit);

    SNLBusNetBit(SNLBusNet* bus, SNLID::Bit bit);
    static void preCreate(const SNLBusNet* bus, SNLID::Bit bit);
    void postCreate();
    void destroyFromBus();
    void destroyFromDesign() override {} //LCOV_EXCL_LINE
    void preDestroy() override;
    void setID(SNLID::DesignObjectID id) override {} //LCOV_EXCL_LINE

    SNLBusNet*  bus_;
    SNLID::Bit  bit_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_BUS_NET_BIT_H_