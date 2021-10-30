#ifndef __SNL_SCALAR_TERM_H_
#define __SNL_SCALAR_TERM_H_

#include "SNLBitTerm.h"
#include "SNLName.h"

namespace SNL {

class SNLScalarTerm final: public SNLBitTerm {
  public:
    friend class SNLDesign;
    using super = SNLBitTerm;

    static SNLScalarTerm* create(SNLDesign* design, const SNLName& name);
    static SNLScalarTerm* create(SNLDesign* design);

    SNLDesign* getDesign() const override { return design_; }

    SNLID getSNLID() const override;
    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    SNLScalarTerm(SNLDesign* design);
    SNLScalarTerm(SNLDesign* design, const SNLName& name);
    static void preCreate(const SNLDesign* design);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    SNLID::DesignObjectID getID() const override { return id_; }
    void setID(SNLID::DesignObjectID id) override { id_ = id; }

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
};

}

#endif /* __SNL_SCALAR_TERM_H_ */ 
