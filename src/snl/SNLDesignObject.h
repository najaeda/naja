#ifndef __SNL_DESIGN_OBJECT_H_
#define __SNL_DESIGN_OBJECT_H_

#include "SNLObject.h"
#include "SNLID.h"

namespace SNL {

class SNLDesign;

class SNLDesignObject: public SNLObject {
  public:
    using super = SNLObject;
    SNLDesignObject(const SNLDesignObject&) = delete;
    SNLDesignObject(const SNLDesignObject&&) = delete;

    virtual SNLDesign* getDesign() const = 0;
    virtual SNLID getSNLID() const = 0;
  protected:
    SNLDesignObject() = default;

    void postCreate();
    void preDestroy() override;
};

}

#endif /* __SNL_DESIGN_OBJECT_H_ */
