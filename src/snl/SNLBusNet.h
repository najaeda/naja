#ifndef __SNL_BUS_NET_H_
#define __SNL_BUS_NET_H_

#include "SNLNet.h"
#include "SNLName.h"

namespace SNL {

class SNLBusNet final: public SNLNet {
  public:
    friend class SNLDesign;
    using super = SNLNet;

    static SNLBusNet* create(
        SNLDesign* design,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }
    ///\return MSB (Most Significant Bit) or left hand side of the bus range.
    SNLID::Bit getMSB() const { return msb_; }
    ///\return LSB (Most Significant Bit) or right hand side of the bus range.
    SNLID::Bit getLSB() const { return lsb_; }

    SNLID::DesignObjectID getID() const override { return id_; }
    SNLID getSNLID() const override;
    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;
  private:
    SNLBusNet(
        SNLDesign* design,
        SNLID::Bit msb,
        SNLID::Bit lsb,
        const SNLName& name);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
    SNLID::Bit                          msb_;
    SNLID::Bit                          lsb_;
};

}

#endif /* __SNL_BUS_NET_H_ */ 
