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

#ifndef __SNL_BUS_TERM_BIT_H_
#define __SNL_BUS_TERM_BIT_H_

#include "SNLBitTerm.h"

namespace SNL {

class SNLBusTerm;

class SNLBusTermBit final: public SNLBitTerm {
  public:
    friend class SNLBusTerm;
    using super = SNLBitTerm;

    SNLDesign* getDesign() const override;

    //\remark returns the owner SNLBusTerm ID
    SNLID::DesignObjectID getID() const override;
    SNLID getSNLID() const override;
    SNLBusTerm* getBus() const { return bus_; }
    SNLID::Bit getBit() const override { return bit_; }
    size_t getPosition() const override;

    constexpr const char* getTypeName() const override;
    SNLName getName() const override;
    SNLTerm::Direction getDirection() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool isAnonymous() const override;

    void destroy() override;
  private:
    static SNLBusTermBit* create(SNLBusTerm* bus, SNLID::Bit bit);

    void setID(SNLID::DesignObjectID id) override {}
    void setPosition(size_t position) override {}

    SNLBusTermBit(SNLBusTerm* bus, SNLID::Bit bit);
    static void preCreate(const SNLBusTerm* bus, SNLID::Bit bit);
    void postCreate();
    void destroyFromBus();
    void destroyFromDesign() override {}
    void preDestroy() override;

    SNLBusTerm* bus_;
    SNLID::Bit  bit_;
};

}

#endif /* __SNL_BUS_TERM_H_ */ 
