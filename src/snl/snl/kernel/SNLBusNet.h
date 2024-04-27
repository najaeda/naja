// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BUS_NET_H_
#define __SNL_BUS_NET_H_

#include <vector>

#include "SNLNet.h"
#include "NajaCollection.h"

namespace naja { namespace SNL {

class SNLBusNetBit;

class SNLBusNet final: public SNLNet {
  public:
    friend class SNLDesign;
    using super = SNLNet;

    /**
     * \brief Create a SNLBusNet.
     * \param design owner SNLDesign.
     * \param msb MSB (Most Significant Bit) or left hand side of the bus range.
     * \param lsb LSB (Most Significant Bit) or right hand side of the bus range.
     * \name optional name.
     * \return created SNLBusNet. 
     */
    static SNLBusNet* create(
        SNLDesign* design,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name=SNLName());
    
    /**
     * \brief Create a SNLBusNet with a given SNLID::DesignObjectID.
     * \param design owner SNLDesign.
     * \param id SNLID::DesignObjectID of the SNLBusNet.
     * \param msb MSB (Most Significant Bit) or left hand side of the bus range.
     * \param lsb LSB (Most Significant Bit) or right hand side of the bus range.
     * \param name optional name.
     * \return created SNLBusNet.
     */
    static SNLBusNet* create(
        SNLDesign* design,
        SNLID::DesignObjectID id,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }
    /// \return MSB (Most Significant Bit) or left hand side of the bus range.
    SNLID::Bit getMSB() const { return msb_; }
    /// \return LSB (Most Significant Bit) or right hand side of the bus range.
    SNLID::Bit getLSB() const { return lsb_; }
    SNLID::Bit getSize() const override;
    SNLBusNetBit* getBit(SNLID::Bit bit) const;
    SNLBusNetBit* getBitAtPosition(size_t position) const;
    NajaCollection<SNLBitNet*> getBits() const override;
    NajaCollection<SNLBusNetBit*> getBusBits() const;
    void insertBits(
        std::vector<SNLBitNet*>& bits,
        std::vector<SNLBitNet*>::const_iterator position,
        SNLID::Bit msb, SNLID::Bit lsb);
    SNLID::DesignObjectID getID() const override { return id_; }
    SNLID getSNLID() const override;
    SNLName getName() const override { return name_; }
    void setName(const SNLName& name) override;
    bool isAnonymous() const override { return name_.empty(); }

    void setType(const Type& type) override;
    bool isAssignConstant() const override;
    virtual bool isSupply0() const override;
    virtual bool isSupply1() const override;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
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
    SNLNet* clone(SNLDesign* design) const override;

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
