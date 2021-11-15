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

    void setID(SNLID::DesignObjectID id) override {}
    SNLID::DesignObjectID getID() const override { return 0; }
    SNLID getSNLID() const override;
    SNLBusTerm* getBus() const { return bus_; }
    SNLID::Bit getBit() const override { return bit_; }

    constexpr const char* getTypeName() const override;
    SNLName getName() const override;
    SNLTerm::Direction getDirection() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool isAnonymous() const override;

    void destroy() override;
  private:
    static SNLBusTermBit* create(SNLBusTerm* bus, SNLID::Bit bit);

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
