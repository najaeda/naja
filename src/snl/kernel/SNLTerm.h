/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SNL_TERM_H_
#define __SNL_TERM_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "SNLNetComponent.h"

namespace naja { namespace SNL {

class SNLTerm: public SNLNetComponent {
  public:
    friend class SNLDesign;
    using super = SNLNetComponent;
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
    virtual size_t getPosition() const = 0;
    virtual SNLName getName() const = 0;
    virtual Direction getDirection() const = 0;

  protected:
    SNLTerm() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    //following used in BusTerm and ScalarTerm
    virtual void setID(SNLID::DesignObjectID id) = 0;
    virtual void setPosition(size_t position) = 0;
    boost::intrusive::set_member_hook<> designTermsHook_  {};
    virtual void destroyFromDesign() = 0;
};

}} // namespace SNL // namespace naja

#endif // __SNL_TERM_H_
