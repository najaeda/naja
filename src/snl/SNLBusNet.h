#ifndef __SNL_BUS_NET_H_
#define __SNL_BUS_NET_H_

#include "SNLNet.h"
#include "SNLName.h"
#include <boost/intrusive/set.hpp>

namespace SNL {

class SNLBusNet final: public SNLNet {
  public:
    friend class SNLDesign;
    using super = SNLNet;

    static SNLBusNet* create(SNLDesign* design, const SNLName& name);

    SNLDesign* getDesign() const override { return design_; }

    SNLID::DesignObjectID getID() const { return id_; }
    SNLID getSNLID() const override;
    SNLName getName() const { return name_; }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;
  private:
    static void preCreate(const SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign();
    void commonPreDestroy();
    void preDestroy() override;
    SNLBusNet(SNLDesign* design, const SNLName& name);

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
    boost::intrusive::set_member_hook<> designBusNetsHook_ {};
};

}

#endif /* __SNL_BUS_NET_H_ */ 
