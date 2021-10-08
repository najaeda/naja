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
    SNLBusTerm(SNLDesign* design, const SNLName& name);

    friend bool operator< (const SNLBusTerm &ln, const SNLBusTerm &rn) {
      return ln.name_ < rn.name_;
    }

    SNLDesign*                          design_;
    SNLName                             name_;
    boost::intrusive::set_member_hook<> designBusTermsHook_ {};
};

}

#endif /* __SNL_BUS_TERM_H_ */ 
