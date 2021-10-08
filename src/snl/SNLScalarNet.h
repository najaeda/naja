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

    SNLScalarNet(SNLDesign* design, const SNLName& name);

    friend bool operator< (const SNLScalarNet &ln, const SNLScalarNet &rn) {
      return ln.name_ < rn.name_;
    }

    SNLDesign*                          design_;
    SNLName                             name_;
    boost::intrusive::set_member_hook<> designScalarNetsHook_ {};
};

}

#endif /* __SNL_SCALAR_NET_H_ */ 
