#ifndef __SNL_NET_H_
#define __SNL_NET_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "SNLDesignObject.h"

namespace SNL {

class SNLNet: public SNLDesignObject {
  public:
    friend class SNLDesign;
    using super = SNLDesignObject;

    virtual SNLID::DesignObjectID getID() const = 0;
    virtual SNLName getName() const = 0;

  protected:
    SNLNet() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    //following used in BusNet and ScalarNet
    virtual void setID(SNLID::DesignObjectID id) = 0;
    boost::intrusive::set_member_hook<> designNetsHook_ {};

    virtual void destroyFromDesign() = 0;
};

}

#endif /* __SNL_NET_H_ */ 
