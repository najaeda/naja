// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_NET_H_
#define __SNL_NET_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "SNLDesignObject.h"

namespace naja { namespace SNL {

class SNLBitNet;

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

        operator const TypeEnum&() const { return typeEnum_; }
        constexpr bool isAssign() const { return typeEnum_ == Assign0 or typeEnum_ == Assign1; }
        constexpr bool isSupply() const { return typeEnum_ == Supply0 or typeEnum_ == Supply1; }
        constexpr bool isConst0() const { return typeEnum_ == Assign0 or typeEnum_ == Supply0; }
        constexpr bool isConst1() const { return typeEnum_ == Assign1 or typeEnum_ == Supply1; }
        constexpr bool isDriving() const { return isAssign() or isSupply(); }
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };

    SNLID::DesignObjectReference getReference() const;
    virtual SNLID::DesignObjectID getID() const = 0;
    ///\return net SNLName
    virtual SNLName getName() const = 0;
    ///\return net size, 1 for SNLScalarNet and SNLBusNetBit
    virtual SNLID::Bit getSize() const = 0;
    virtual NajaCollection<SNLBitNet*> getBits() const = 0;

    virtual void setType(const Type& type) = 0;
    ///\return true if all bits of this net are assigned to 1'b0 or 1'b1
    virtual bool isAssignConstant() const = 0;

  protected:
    SNLNet() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    virtual void destroyFromDesign() = 0;
    virtual SNLNet* clone(SNLDesign* design) const = 0;

    //following used in BusNet and ScalarNet
    virtual void setID(SNLID::DesignObjectID id) = 0;
    boost::intrusive::set_member_hook<> designNetsHook_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_NET_H_
