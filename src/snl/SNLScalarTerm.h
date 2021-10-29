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
    static SNLScalarTerm* create(SNLDesign* design);

    SNLDesign* getDesign() const override { return design_; }

    SNLID getSNLID() const override;
    SNLName getName() const { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    SNLScalarTerm(SNLDesign* design, const SNLName& name);
    SNLScalarTerm(SNLDesign* design);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    static void preCreate(const SNLDesign* design);
    void postCreate();
    void destroyFromDesign();
    void commonPreDestroy();
    void preDestroy() override;

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
    boost::intrusive::set_member_hook<> designScalarTermsHook_ {};
};

}

#endif /* __SNL_SCALAR_TERM_H_ */ 
