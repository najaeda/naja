#ifndef __SNL_SCALAR_NET_H_
#define __SNL_SCALAR_NET_H_

#include <range/v3/view/transform.hpp>

#include "SNLBitNet.h"
#include "SNLName.h"

namespace SNL {

class SNLScalarNet final: public SNLBitNet {
  public:
    friend class SNLDesign;
    using super = SNLBitNet;

    static SNLScalarNet* create(SNLDesign* design, const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }

    SNLID getSNLID() const override;
    SNLID::DesignObjectID getID() const override { return id_; }
    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;
  private:
    SNLScalarNet(SNLDesign* design, const SNLName& name);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
};

}

#endif /* __SNL_SCALAR_NET_H_ */ 
