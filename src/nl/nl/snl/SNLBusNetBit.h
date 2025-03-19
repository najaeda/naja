// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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
    NLID::DesignObjectID getID() const override;
    NLID getNLID() const override;
    SNLBusNet* getBus() const { return bus_; }
    NLID::Bit getBit() const { return bit_; }
    NajaCollection<SNLBitNet*> getBits() const override;

    const char* getTypeName() const override;
    NLName getName() const override;
    bool isAnonymous() const override;
    void setName(const NLName& name) override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool deepCompare(const SNLNet* other, std::string& reason) const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
  private:
    static SNLBusNetBit* create(SNLBusNet* bus, NLID::Bit bit);

    SNLBusNetBit(SNLBusNet* bus, NLID::Bit bit);
    static void preCreate(const SNLBusNet* bus, NLID::Bit bit);
    void postCreate();
    void destroyFromBus();
    void destroyFromDesign() override {} //LCOV_EXCL_LINE
    void commonPreDestroy();
    void preDestroy() override;
    SNLNet* clone(SNLDesign* design) const override { return nullptr; } //LCOV_EXCL_LINE
    void setID(NLID::DesignObjectID id) override {} //LCOV_EXCL_LINE

    SNLBusNet*  bus_;
    NLID::Bit   bit_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_BUS_NET_BIT_H_
