#ifndef __SNL_NET_COMPONENT_H_
#define __SNL_NET_COMPONENT_H_

#include "SNLDesignObject.h"

namespace SNL {

class SNLNetComponent: public SNLDesignObject {
  public:
    using super = SNLDesignObject;
  protected:
    SNLNetComponent() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;
};

}

#endif /* __SNL_NET_COMPONENT_H_ */
