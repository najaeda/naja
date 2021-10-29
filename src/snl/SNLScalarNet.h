#ifndef __SNL_SCALAR_NET_H_
#define __SNL_SCALAR_NET_H_

#include "SNLBitNet.h"
#include "SNLName.h"
#include <boost/intrusive/set.hpp>

namespace SNL {

class SNLScalarNet: public SNLBitNet {
  public:
    friend class SNLDesign;
    using super = SNLBitNet;

    static SNLScalarNet* create(SNLDesign* design, const SNLName& name);

    SNLDesign* getDesign() const override { return design_; }

    SNLID getSNLID() const override;
    SNLName getName() const { return name_; }
    bool isAnonymous() const { return name_.empty(); }
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

    SNLScalarNet(SNLDesign* design, const SNLName& name);

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
    boost::intrusive::set_member_hook<> designScalarNetsHook_ {};
};

}

#endif /* __SNL_SCALAR_NET_H_ */ 
