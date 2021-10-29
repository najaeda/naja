#ifndef __SNL_BUS_TERM_H_
#define __SNL_BUS_TERM_H_

#include "SNLTerm.h"
#include "SNLName.h"
#include <boost/intrusive/set.hpp>

namespace SNL {

class SNLBusTerm: public SNLTerm {
  public:
    friend class SNLDesign;
    using super = SNLTerm;

    static SNLBusTerm* create(SNLDesign* design, const SNLName& name);

    SNLDesign* getDesign() const override { return design_; }

    SNLID::DesignObjectID getID() const { return id_; }
    SNLID getSNLID() const override;
    SNLName getName() const { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    static void preCreate(const SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign();
    void commonPreDestroy();
    void preDestroy() override;
    SNLBusTerm(SNLDesign* design, const SNLName& name);

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
    boost::intrusive::set_member_hook<> designBusTermsHook_ {};
};

}

#endif /* __SNL_BUS_TERM_H_ */ 
