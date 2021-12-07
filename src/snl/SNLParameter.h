#ifndef __SNL_PARAMETER_H_
#define __SNL_PARAMETER_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"

namespace SNL {

class SNLDesign;

class SNLParameter {
  public:
    friend class SNLDesign;

    static SNLParameter* create(SNLDesign* design, const SNLName& name, const SNLName& value);
  private:
    SNLParameter(const SNLName& name, const SNLName& value);
    static void preCreate(SNLDesign* design, const SNLName& name);
    void postCreate();

    SNLName                             name_;
    SNLName                             value_;
    boost::intrusive::set_member_hook<> designParametersHook_;
};

}

#endif /* __SNL_PARAMETER_H_ */
