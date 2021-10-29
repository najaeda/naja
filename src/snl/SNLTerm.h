#ifndef __SNL_TERM_H_
#define __SNL_TERM_H_

#include "SNLDesignObject.h"

namespace SNL {

class SNLTerm: public SNLDesignObject {
  public:
    friend class SNLDesign;
    using super = SNLDesignObject;

    virtual bool isAnonymous() const = 0;

  protected:
    SNLTerm() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;
};

}

#endif /* __SNL_TERM_H_ */ 
