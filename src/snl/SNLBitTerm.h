#ifndef __SNL_BIT_TERM_H_
#define __SNL_BIT_TERM_H_

#include "SNLTerm.h"

namespace SNL {

class SNLBitTerm: public SNLTerm {
  public:
    using super = SNLTerm;

    virtual SNLID::Bit getBit() const = 0;
  protected:
    SNLBitTerm() = default;
    static void preCreate();
    void postCreate();
    void preDestroy() override;
};

}

#endif /* __SNL_BIT_TERM_H_ */ 
