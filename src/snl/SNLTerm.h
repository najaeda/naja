#ifndef __SNL_TERM_H_
#define __SNL_TERM_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "SNLDesignObject.h"

namespace SNL {

class SNLTerm: public SNLDesignObject {
  public:
    friend class SNLDesign;
    using super = SNLDesignObject;
    class Direction {
      public:
        enum DirectionEnum {
          Input, Output, InOut
        };
        Direction(const DirectionEnum& dirEnum);
        Direction(const Direction& direction) = default;
        operator const DirectionEnum&() const {return dirEnum_;}
        std::string getString() const;
        private:
          DirectionEnum dirEnum_;
    };

    virtual SNLID::DesignObjectID getID() const = 0;
    virtual SNLName getName() const = 0;
    virtual bool isAnonymous() const = 0;
    virtual Direction getDirection() const = 0;

  protected:
    SNLTerm() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    //following used in BusTerm and ScalarTerm
    virtual void setID(SNLID::DesignObjectID id) = 0;
    boost::intrusive::set_member_hook<> designTermsHook_  {};
    //

    virtual void destroyFromDesign() = 0;
};

}

#endif /* __SNL_TERM_H_ */ 