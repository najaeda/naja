// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BUS_NET_H_
#define __SNL_BUS_NET_H_

#include <vector>

#include "SNLNet.h"
#include "NajaCollection.h"

namespace naja { namespace NL {

class SNLBusNetBit;

class SNLBusNet final: public SNLNet {
  public:
    friend class SNLDesign;
    friend class SNLBusNetBit;
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
        NLID::Bit msb,
        NLID::Bit lsb,
        const NLName& name=NLName());
    
    /**
     * \brief Create a SNLBusNet with a given NLID::DesignObjectID.
     * \param design owner SNLDesign.
     * \param id NLID::DesignObjectID of the SNLBusNet.
     * \param msb MSB (Most Significant Bit) or left hand side of the bus range.
     * \param lsb LSB (Most Significant Bit) or right hand side of the bus range.
     * \param name optional name.
     * \return created SNLBusNet.
     */
    static SNLBusNet* create(
        SNLDesign* design,
        NLID::DesignObjectID id,
        NLID::Bit msb,
        NLID::Bit lsb,
        const NLName& name=NLName());

    SNLDesign* getDesign() const override { return design_; }
    /// \return MSB (Most Significant Bit) or left hand side of the bus range.
    NLID::Bit getMSB() const { return msb_; }
    /// \return LSB (Most Significant Bit) or right hand side of the bus range.
    NLID::Bit getLSB() const { return lsb_; }
    NLID::Bit getWidth() const override;
    SNLBusNetBit* getBit(NLID::Bit bit) const;
    SNLBusNetBit* getBitAtPosition(size_t position) const;
    size_t getBitPosition(NLID::Bit bit) const;
    NajaCollection<SNLBitNet*> getBits() const override;
    NajaCollection<SNLBusNetBit*> getBusBits() const;
    NLID::DesignObjectID getID() const override { return id_; }
    NLID getNLID() const override;
    NLName getName() const override { return name_; }
    void setName(const NLName& name) override;
    bool isUnnamed() const override { return name_.empty(); }

    void insertBits(
        std::vector<SNLBitNet*>& bits,
        std::vector<SNLBitNet*>::const_iterator position,
        NLID::Bit msb, NLID::Bit lsb);

    void setType(const Type& type) override;
    bool isAllNull() const;
    bool isAssign0() const override;
    bool isAssign1() const override;
    bool isSupply0() const override;
    bool isSupply1() const override;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool deepCompare(const SNLNet* other, std::string& reason) const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
  private:
    SNLBusNet(
        SNLDesign* design,
        NLID::Bit msb,
        NLID::Bit lsb,
        const NLName& name);
    SNLBusNet(
        SNLDesign* design,
        NLID::DesignObjectID id,
        NLID::Bit msb,
        NLID::Bit lsb,
        const NLName& name);
    static void preCreate(const SNLDesign* design, const NLName& name);
    static void preCreate(const SNLDesign* design, NLID::DesignObjectID id, const NLName& name);
    void createBits();
    void postCreateAndSetID();
    void postCreate();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;
    void removeBit(SNLBusNetBit* bit);
    SNLNet* clone(SNLDesign* design) const override;

    void setID(NLID::DesignObjectID id) override { id_ = id; }

    using Bits = std::vector<SNLBusNetBit*>;

    SNLDesign*            design_;
    NLID::DesignObjectID  id_;
    NLName                name_   {};
    NLID::Bit             msb_;
    NLID::Bit             lsb_;
    Bits                  bits_   {};
};

}} // namespace NL // namespace naja

#endif // __SNL_BUS_NET_H_
