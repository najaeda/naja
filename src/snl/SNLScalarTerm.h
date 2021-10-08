#ifndef __SNL_SCALAR_TERM_H_
#define __SNL_SCALAR_TERM_H_

#include "SNLBitTerm.h"
#include "SNLName.h"
#include <boost/intrusive/set.hpp>

namespace SNL {

class SNLScalarTerm: public SNLBitTerm {
  public:
    friend class SNLDesign;
    using super = SNLBitTerm;

    static SNLScalarTerm* create(SNLDesign* design, const SNLName& name);

    SNLDesign* getDesign() const override { return design_; }

    SNLName getName() const { return name_; }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    static void preCreate(const SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign();
    void commonPreDestroy();
    void preDestroy() override;

    SNLScalarTerm(SNLDesign* design, const SNLName& name);

    friend bool operator< (const SNLScalarTerm &ln, const SNLScalarTerm &rn) {
      return ln.name_ < rn.name_;
    }

    SNLDesign*                          design_;
    SNLName                             name_;
    boost::intrusive::set_member_hook<> designScalarTermsHook_ {};
};

}

#endif /* __SNL_SCALAR_TERM_H_ */ 
