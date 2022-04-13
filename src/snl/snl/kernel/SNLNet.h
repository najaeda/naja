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

#ifndef __SNL_NET_H_
#define __SNL_NET_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "SNLDesignObject.h"

namespace naja { namespace SNL {

class SNLNet: public SNLDesignObject {
  public:
    friend class SNLDesign;
    using super = SNLDesignObject;

    class Type {
      public:
        enum TypeEnum {
          Standard, Assign0, Assign1, Supply0, Supply1
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type&) = default;
        Type& operator=(const Type&) = default;

        operator const TypeEnum&() const {return typeEnum_;}
        constexpr bool isAssign() { return typeEnum_ == Assign0 or typeEnum_ == Assign1; }
        constexpr bool isSupply() { return typeEnum_ == Supply0 or typeEnum_ == Supply1; }
        constexpr bool isConst0() { return typeEnum_ == Assign0 or typeEnum_ == Supply0; }
        constexpr bool isConst1() { return typeEnum_ == Assign1 or typeEnum_ == Supply1; }
        constexpr bool isDriving() { return isAssign() or isSupply(); }
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };

    virtual SNLID::DesignObjectID getID() const = 0;
    ///\return net SNLName
    virtual SNLName getName() const = 0;
    ///\return net size, 1 for SNLScalarNet and SNLBusNetBit
    virtual size_t getSize() const = 0;

    virtual void setType(const Type& type) = 0;

  protected:
    SNLNet() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    virtual void destroyFromDesign() = 0;

    //following used in BusNet and ScalarNet
    virtual void setID(SNLID::DesignObjectID id) = 0;
    boost::intrusive::set_member_hook<> designNetsHook_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_NET_H_
