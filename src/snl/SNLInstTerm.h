#ifndef __SNL_INSTTERM_H_
#define __SNL_INSTTERM_H_

#include "SNLTerm.h"

namespace SNL {

class SNLInstance;
class SNLBitTerm;

class SNLInstTerm final: public SNLNetComponent {
  public:
    friend class SNLInstance;
    using super = SNLNetComponent;
    SNLInstTerm() = delete;

    SNLDesign* getDesign() const override;

    SNLID getSNLID() const override;

    bool isAnonymous() const override;
    SNLTerm::Direction getDirection() const;
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    SNLInstance* getInstance() const { return instance_; }
    SNLBitTerm* getTerm() const { return term_; }

  private:
    SNLInstTerm(SNLInstance* instance, SNLBitTerm* term);
    static SNLInstTerm* create(SNLInstance* instance, SNLBitTerm* term);
    static void preCreate(const SNLInstance* instance, const SNLBitTerm* term);
    void postCreate();
    void preDestroy() override;

    SNLInstance*  instance_;
    SNLBitTerm*   term_;
};

}

#endif /* __SNL_INSTTERM_H_ */
