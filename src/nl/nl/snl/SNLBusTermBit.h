// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BUS_TERM_BIT_H_
#define __SNL_BUS_TERM_BIT_H_

#include "SNLBitTerm.h"

namespace naja { namespace SNL {

class SNLBusTerm;

class SNLBusTermBit final: public SNLBitTerm {
  public:
    friend class SNLBusTerm;
    using super = SNLBitTerm;

    SNLDesign* getDesign() const override;

    NLID::DesignObjectID getID() const override;
    NLID getNLID() const override;
    ///\return this SNLBusTermBit owner SNLBusTerm.
    SNLBusTerm* getBus() const { return bus_; }
    NLID::Bit getBit() const override { return bit_; }
    size_t getFlatID() const override;
    ///\return the position of this SNLBusTermBit in SNLBusTerm bits vector.
    size_t getPositionInBus() const;
    NajaCollection<SNLBitTerm*> getBits() const override;

    const char* getTypeName() const override;
    NLName getName() const override;
    bool isAnonymous() const override;
    void setName(const NLName& name) override;
    SNLTerm::Direction getDirection() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool deepCompare(const SNLTerm* other, std::string& reason) const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    void destroy() override;
  private:
    static SNLBusTermBit* create(SNLBusTerm* bus, NLID::Bit bit);

    void setID(NLID::DesignObjectID id) override {} //LCOV_EXCL_LINE
    void setFlatID(size_t position) override {} //LCOV_EXCL_LINE

    SNLBusTermBit(SNLBusTerm* bus, NLID::Bit bit);
    static void preCreate(const SNLBusTerm* bus, NLID::Bit bit);
    void postCreate();
    void destroyFromBus();
    void destroyFromDesign() override {} //LCOV_EXCL_LINE
    SNLTerm* clone(SNLDesign* design) const override { return nullptr; } //LCOV_EXCL_LINE
    void preDestroy() override;

    SNLBusTerm* bus_;
    NLID::Bit   bit_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_BUS_TERM_H_
