#ifndef __SNL_BIT_NET_H_
#define __SNL_BIT_NET_H_

#include "SNLNet.h"

namespace SNL {

class SNLBitNet: public SNLNet {
  public:
    using super = SNLNet;

  protected:
    SNLBitNet() = default;
    static void preCreate();
    void postCreate();
    void preDestroy() override;
};

}

#endif /* __SNL_BIT_NET_H_ */ 
