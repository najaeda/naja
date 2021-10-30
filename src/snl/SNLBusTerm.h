#ifndef __SNL_BUS_TERM_H_
#define __SNL_BUS_TERM_H_

#include "SNLTerm.h"
#include "SNLName.h"

namespace SNL {

class SNLBusTerm final: public SNLTerm {
  public:
    friend class SNLDesign;
    using super = SNLTerm;

    static SNLBusTerm* create(SNLDesign* design, const SNLName& name, const Direction& direction);
    static SNLBusTerm* create(SNLDesign* design, const Direction& direction);

    SNLDesign* getDesign() const override { return design_; }

    SNLID::DesignObjectID getID() const override { return id_; }
    SNLID getSNLID() const override;
    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    SNLTerm::Direction getDirection() const override { return direction_; }
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    SNLBusTerm(SNLDesign* design, const Direction& direction);
    SNLBusTerm(SNLDesign* design, const SNLName& name, const Direction& direction);
    static void preCreate(const SNLDesign* design);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
    SNLTerm::Direction                  direction_;
};

}

#endif /* __SNL_BUS_TERM_H_ */ 
