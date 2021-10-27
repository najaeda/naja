#ifndef __SNL_INSTANCE_H_
#define __SNL_INSTANCE_H_

#include <vector>
#include <boost/intrusive/set.hpp>
#include "SNLDesignObject.h"
#include "SNLName.h"

namespace SNL {

class SNLInstTerm;

class SNLInstance final: public SNLDesignObject {
  public:
    friend class SNLDesign;
    using super = SNLDesignObject;
    using SNLInstanceInstTerms = std::vector<SNLInstTerm*>; 

    static SNLInstance* create(SNLDesign* design, SNLDesign* model, const SNLName& name);

    SNLDesign* getDesign() const override { return design_; }
    SNLDesign* getModel() const { return model_; }

    SNLName getName() const { return name_; }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;
  private:
    static void preCreate(SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign();
    void commonPreDestroy();
    void preDestroy() override;

    SNLInstance(SNLDesign* design, SNLDesign* model, const SNLName& name);

    friend bool operator< (const SNLInstance &li, const SNLInstance &ri) {
      return li.name_ < ri.name_;
    }

    SNLDesign*                          design_			{nullptr};
    SNLDesign*                          model_			{nullptr};
    SNLName                             name_;
    SNLInstanceInstTerms		instTerms_		{};
    boost::intrusive::set_member_hook<> designInstancesHook_	{};
};

}

#endif /* __SNL_INSTANCE_H_ */ 
