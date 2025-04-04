// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BUS_TERM_H_
#define __SNL_BUS_TERM_H_

#include <vector>

#include "SNLTerm.h"
#include "NajaCollection.h"

namespace naja { namespace NL {

class SNLNet;
class SNLBusTermBit;

class SNLBusTerm final: public SNLTerm {
  public:
    friend class SNLDesign;
    using super = SNLTerm;

    /**
     * \brief Create a SNLBusTerm.
     * \param design owner SNLDesign.
     * \param direction direction of the term.
     * \param msb MSB (Most Significant Bit) or left hand side of the bus range.
     * \param lsb LSB (Most Significant Bit) or right hand side of the bus range.
     * \param name optional name.
     * \return created SNLBusTerm.  
     */
    static SNLBusTerm* create(
        SNLDesign* design,
        Direction direction,
        NLID::Bit msb,
        NLID::Bit lsb,
        const NLName& name=NLName());

    /**
     * \brief Create a SNLBusTerm with a given NLID::DesignObjectID.
     * \param design owner SNLDesign.
     * \param id NLID::DesignObjectID of the instance.
     * \param direction direction of the term.
     * \param msb MSB (Most Significant Bit) or left hand side of the bus range.
     * \param lsb LSB (Most Significant Bit) or right hand side of the bus range.
     * \param name optional name.
     * \return created SNLBusTerm. 
     */
    static SNLBusTerm* create(
        SNLDesign* design,
        NLID::DesignObjectID id,
        Direction direction,
        NLID::Bit msb,
        NLID::Bit lsb,
        const NLName& name=NLName());

    SNLBitNet* getNet() const override { return nullptr; }
    void setNet(SNLNet* net) override;

    SNLDesign* getDesign() const override { return design_; }
    
    ///\return MSB (Most Significant Bit) or left hand side of the bus range.
    NLID::Bit getMSB() const { return msb_; }
    ///\return LSB (Most Significant Bit) or right hand side of the bus range.
    NLID::Bit getLSB() const { return lsb_; }
    NLID::Bit getWidth() const override;
    SNLBusTermBit* getBit(NLID::Bit bit) const;
    SNLBusTermBit* getBitAtPosition(size_t position) const;
    NajaCollection<SNLBitTerm*> getBits() const override;
    NajaCollection<SNLBusTermBit*> getBusBits() const;

    NLID::DesignObjectID getID() const override { return id_; }
    NLID getNLID() const override;
    size_t getFlatID() const override { return flatID_; }
    NLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    void setName(const NLName& name) override;
   
    SNLTerm::Direction getDirection() const override { return direction_; }
    void setDirection(const SNLTerm::Direction& direction) { direction_ = direction; }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool deepCompare(const SNLTerm* other, std::string& reason) const override;
    
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

  private:
    SNLBusTerm(
        SNLDesign* design,
        Direction direction,
        NLID::Bit msb,
        NLID::Bit lsb,
        const NLName& name);
    SNLBusTerm(
        SNLDesign* design,
        NLID::DesignObjectID id,
        Direction direction,
        NLID::Bit msb,
        NLID::Bit lsb,
        const NLName& name);
    static void preCreate(const SNLDesign* design, const NLName& name);
    static void preCreate(const SNLDesign* design, NLID::DesignObjectID id, const NLName& name);
    void createBits();
    void postCreate();
    void postCreateAndSetID();
    void destroyFromDesign() override;
    SNLTerm* clone(SNLDesign* design) const override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(NLID::DesignObjectID id) override { id_ = id; }
    void setFlatID(size_t flatID) override { flatID_ = flatID; }

    using Bits = std::vector<SNLBusTermBit*>;

    SNLDesign*              design_;
    NLID::DesignObjectID    id_;
    size_t                  flatID_   {0};
    NLName                  name_     {};
    SNLTerm::Direction      direction_;
    NLID::Bit               msb_;
    NLID::Bit               lsb_;
    Bits                    bits_     {};
};

}} // namespace NL // namespace naja

#endif // __SNL_BUS_TERM_H_