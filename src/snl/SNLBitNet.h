#ifndef __SNL_BIT_NET_H_
#define __SNL_BIT_NET_H_

#include <range/v3/view/all.hpp>
#include <range/v3/view/transform.hpp>

#include "SNLNet.h"
#include "SNLNetComponent.h"

namespace SNL {

class SNLBitNet: public SNLNet {
  public:
    friend class SNLNetComponent;
    using super = SNLNet;

    auto getComponents() const {
      return ranges::views::all(components_)
        | ranges::views::transform([](const SNLNetComponent& c) { return const_cast<SNLNetComponent*>(&c); });
    }
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
