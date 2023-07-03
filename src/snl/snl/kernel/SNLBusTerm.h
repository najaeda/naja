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

#ifndef __SNL_BUS_TERM_H_
#define __SNL_BUS_TERM_H_

#include <vector>

#include "SNLTerm.h"
#include "NajaCollection.h"

namespace naja { namespace SNL {

class SNLNet;
class SNLBusTermBit;

class SNLBusTerm final: public SNLTerm {
  public:
    friend class SNLDesign;
    using super = SNLTerm;

    static SNLBusTerm* create(
        SNLDesign* design,
        Direction direction,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name=SNLName());
    static SNLBusTerm* create(
        SNLDesign* design,
        SNLID::DesignObjectID id,
        Direction direction,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name=SNLName());

    SNLBitNet* getNet() const override { return nullptr; }
    void setNet(SNLNet* net) override;

    SNLDesign* getDesign() const override { return design_; }
    
    ///\return MSB (Most Significant Bit) or left hand side of the bus range.
    SNLID::Bit getMSB() const { return msb_; }
    ///\return LSB (Most Significant Bit) or right hand side of the bus range.
    SNLID::Bit getLSB() const { return lsb_; }
    SNLID::Bit getSize() const override;
    SNLBusTermBit* getBit(SNLID::Bit bit) const;
    SNLBusTermBit* getBitAtPosition(size_t position) const;
    NajaCollection<SNLBitTerm*> getBits() const override;
    NajaCollection<SNLBusTermBit*> getBusBits() const;

    SNLID::DesignObjectID getID() const override { return id_; }
    SNLID getSNLID() const override;
    size_t getFlatID() const override { return flatID_; }
    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    void setName(const SNLName& name);
    const char* getTypeName() const override;
    SNLTerm::Direction getDirection() const override { return direction_; }
    std::string getString() const override;
    std::string getDescription() const override;

  private:
    SNLBusTerm(
        SNLDesign* design,
        Direction direction,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name);
    SNLBusTerm(
        SNLDesign* design,
        SNLID::DesignObjectID id,
        Direction direction,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    static void preCreate(const SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name);
    void createBits();
    void postCreate();
    void postCreateAndSetID();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }
    void setFlatID(size_t flatID) override { flatID_ = flatID; }

    using Bits = std::vector<SNLBusTermBit*>;

    SNLDesign*              design_;
    SNLID::DesignObjectID   id_;
    size_t                  flatID_   {0};
    SNLName                 name_     {};
    SNLTerm::Direction      direction_;
    SNLID::Bit              msb_;
    SNLID::Bit              lsb_;
    Bits                    bits_     {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_BUS_TERM_H_