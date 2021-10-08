#ifndef __SNL_NET_H_
#define __SNL_NET_H_

#include "SNLDesignObject.h"

namespace SNL {

class SNLNet: public SNLDesignObject {
  public:
    friend class SNLDesign;
    using super = SNLDesignObject;

  protected:
    SNLNet() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;
};

}

#endif /* __SNL_NET_H_ */ 
