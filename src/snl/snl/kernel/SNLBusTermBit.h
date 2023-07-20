// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
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

    ///\return the owner SNLBusTerm ID.
    SNLID::DesignObjectID getID() const override;
    SNLID getSNLID() const override;
    ///\return this SNLBusTermBit owner SNLBusTerm.
    SNLBusTerm* getBus() const { return bus_; }
    SNLID::Bit getBit() const override { return bit_; }
    size_t getFlatID() const override;
    ///\return the position of this SNLBusTermBit in SNLBusTerm bits vector.
    size_t getPositionInBus() const;
    NajaCollection<SNLBitTerm*> getBits() const override;

    const char* getTypeName() const override;
    SNLName getName() const override;
    SNLTerm::Direction getDirection() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool isAnonymous() const override;

    void destroy() override;
  private:
    static SNLBusTermBit* create(SNLBusTerm* bus, SNLID::Bit bit);

    void setID(SNLID::DesignObjectID id) override {} //LCOV_EXCL_LINE
    void setFlatID(size_t position) override {} //LCOV_EXCL_LINE

    SNLBusTermBit(SNLBusTerm* bus, SNLID::Bit bit);
    static void preCreate(const SNLBusTerm* bus, SNLID::Bit bit);
    void postCreate();
    void destroyFromBus();
    void destroyFromDesign() override {} //LCOV_EXCL_LINE
    void preDestroy() override;

    SNLBusTerm* bus_;
    SNLID::Bit  bit_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_BUS_TERM_H_
