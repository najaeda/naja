#ifndef __SNL_BUS_NET_BIT_H_
#define __SNL_BUS_NET_BIT_H_

#include "SNLBitNet.h"

namespace SNL {

class SNLBusNet;

class SNLBusNetBit final: public SNLBitNet {
  public:
    friend class SNLBusNet;
    using super = SNLBitNet;

    SNLDesign* getDesign() const override;

    void setID(SNLID::DesignObjectID id) override {}
    //\remark returns the owner SNLBusNet ID
    SNLID::DesignObjectID getID() const override;
    SNLID getSNLID() const override;
    SNLBusNet* getBus() const { return bus_; }
    SNLID::Bit getBit() const { return bit_; }

    constexpr const char* getTypeName() const override;
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
    void destroyFromDesign() override {}
    void preDestroy() override;

    SNLBusNet*  bus_;
    SNLID::Bit  bit_;
};

}

#endif /* __SNL_BUS_NET_BIT_H_ */ 
