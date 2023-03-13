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

#ifndef __SNL_BUS_NET_H_
#define __SNL_BUS_NET_H_

#include <vector>

#include "SNLNet.h"
#include "SNLName.h"
#include "NajaCollection.h"

namespace naja { namespace SNL {

class SNLBusNetBit;

class SNLBusNet final: public SNLNet {
  public:
    friend class SNLDesign;
    using super = SNLNet;

    static SNLBusNet* create(
        SNLDesign* design,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name=SNLName());
    
    static SNLBusNet* create(
        SNLDesign* design,
        SNLID::DesignObjectID id,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }
    ///\return MSB (Most Significant Bit) or left hand side of the bus range.
    SNLID::Bit getMSB() const { return msb_; }
    ///\return LSB (Most Significant Bit) or right hand side of the bus range.
    SNLID::Bit getLSB() const { return lsb_; }
    SNLID::Bit getSize() const override;
    SNLBusNetBit* getBit(SNLID::Bit bit) const;
    SNLBusNetBit* getBitAtPosition(size_t position) const;
    NajaCollection<SNLBusNetBit*> getBits() const;

    SNLID::DesignObjectID getID() const override { return id_; }
    SNLID getSNLID() const override;
    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }

    void setType(const Type& type) override;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    SNLBusNet(
        SNLDesign* design,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name);
    SNLBusNet(
        SNLDesign* design,
        SNLID::DesignObjectID id,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    static void preCreate(const SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name);
    void createBits();
    void postCreateAndSetID();
    void postCreate();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }

    using Bits = std::vector<SNLBusNetBit*>;

    SNLDesign*            design_;
    SNLID::DesignObjectID id_;
    SNLName               name_   {};
    SNLID::Bit            msb_;
    SNLID::Bit            lsb_;
    Bits                  bits_   {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_BUS_NET_H_
