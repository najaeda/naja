#ifndef __SNL_NET_COMPONENT_H_
#define __SNL_NET_COMPONENT_H_

#include <boost/intrusive/set.hpp>

#include "SNLDesignObject.h"

namespace SNL {

class SNLBitNet;

class SNLNetComponent: public SNLDesignObject {
  public:
    friend class SNLBitNet;
    using super = SNLDesignObject;

    SNLBitNet* getNet() const { return net_; }
    void setNet(SNLBitNet* net);

  protected:
    SNLNetComponent() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    SNLBitNet*  net_                                        { nullptr };
    boost::intrusive::set_member_hook<> netComponentsHook_  {};
};

}

#endif /* __SNL_NET_COMPONENT_H_ */
