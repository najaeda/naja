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
    void destroy();
    SNLDesign* getDesign() const { return design_; }
    SNLName getName() const { return name_; }
    std::string getValue() const { return value_; }

    friend bool operator< (const SNLParameter& lp, const SNLParameter& rp) {
      return lp.getName() < rp.getName();
    }
    struct SNLParameterComp {
      bool operator()(const SNLName& ln, const SNLParameter& rp) const {
        return ln < rp.getName();
      }
      bool operator()(const SNLParameter& lp, const SNLName& rn) const {
        return lp.getName() < rn;
      }
    };
  private:
    SNLParameter(const SNLName& name, const SNLName& value);
    static void preCreate(SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign();

    SNLDesign*                          design_;
    SNLName                             name_;
    std::string                         value_;
    boost::intrusive::set_member_hook<> designParametersHook_;
};

}

#endif /* __SNL_PARAMETER_H_ */
