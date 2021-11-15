#ifndef __SNL_BIT_NET_H_
#define __SNL_BIT_NET_H_

#include "SNLNet.h"
#include "SNLNetComponent.h"

namespace SNL {

class SNLBitNet: public SNLNet {
  public:
    friend class SNLNetComponent;
    using super = SNLNet;

  protected:
    SNLBitNet() = default;
    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    void addComponent(SNLNetComponent* net);
    void removeComponent(SNLNetComponent* net);

    using SNLBitNetComponentsHook =
      boost::intrusive::member_hook<SNLNetComponent, boost::intrusive::set_member_hook<>, &SNLNetComponent::netComponentsHook_>;
    using SNLBitNetComponents = boost::intrusive::set<SNLNetComponent, SNLBitNetComponentsHook>;

    SNLBitNetComponents components_;
};

}

#endif /* __SNL_BIT_NET_H_ */ 
