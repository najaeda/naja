// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_NET_H_
#define __SNL_NET_H_

#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "SNLDesignObject.h"

namespace naja { namespace NL {

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
        bool operator==(const Type& other) const {
          return typeEnum_ == other.typeEnum_;
        }
        bool operator==(const TypeEnum& otherEnum) const {
          return typeEnum_ == otherEnum;
        }

        operator const TypeEnum&() const { return typeEnum_; }
        constexpr bool isAssign0() const { return typeEnum_ == Assign0;  }
        constexpr bool isAssign1() const { return typeEnum_ == Assign1;  }
        constexpr bool isAssign() const { return isAssign0() or isAssign1(); }
        constexpr bool isSupply0() const { return typeEnum_ == Supply0; }
        constexpr bool isSupply1() const { return typeEnum_ == Supply1; }
        constexpr bool isSupply() const { return isSupply0() or isSupply1(); }
        constexpr bool isConst0() const { return typeEnum_ == Assign0 or typeEnum_ == Supply0; }
        constexpr bool isConst1() const { return typeEnum_ == Assign1 or typeEnum_ == Supply1; }
        constexpr bool isDriving() const { return isAssign() or isSupply(); }
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };

    NLID::DesignObjectReference getReference() const;
    /// \return this SNLNet unique ID in parent SNLDesign.
    virtual NLID::DesignObjectID getID() const = 0;

    /// \return net NLName.
    virtual NLName getName() const = 0;
    
    /// \return net width, 1 for SNLScalarNet and SNLBusNetBit.
    virtual NLID::Bit getWidth() const = 0;

    /**
     * \return this SNLNet SNLBitNet collection.
     * 
     * Will return itself for SNLScalarNet and SNLBusNetBit and a collection
     * of SNLBusNetBit for SNLBusNet.
     */ 
    virtual NajaCollection<SNLBitNet*> getBits() const = 0;

    /// \brief Change this SNLNet type. 
    virtual void setType(const Type& type) = 0;

    /// \return true if all bits of this net are assigned to 1'b0.
    virtual bool isAssign0() const = 0;
    /// \return true if all bits of this net are assigned to 1'b1.
    virtual bool isAssign1() const = 0;
    /// \return true if all bits of this net are assigned to 1'b0 or 1'b1.
    bool isAssignConstant() const { return isAssign0() or isAssign1(); }
    /// \return true if all bits of this net are of type Supply0
    virtual bool isSupply0() const = 0;
    /// \return true if all bits of this net are of type Supply1
    virtual bool isSupply1() const = 0;
    /// \return true if all bits of this net are of type Supply0 or Supply1
    bool isSupply() const { return isSupply0() or isSupply1(); }
    /// \return true if all bits of this net are constants 0
    bool isConstant0() const { return isAssign0() or isSupply0(); }
    /// \return true if all bits of this net are constants 1
    bool isConstant1() const { return isAssign1() or isSupply1(); }
    /// \return true if all bits of this net are constants
    bool isConstant() const { return isConstant0() or isConstant1(); }


    virtual bool deepCompare(const SNLNet* other, std::string& reason) const = 0;

  protected:
    SNLNet() = default;

    static void preCreate();
    void postCreate() override;
    void preDestroy() override;

  private:
    virtual void destroyFromDesign() = 0;
    virtual SNLNet* clone(SNLDesign* design) const = 0;

    //following used in BusNet and ScalarNet
    virtual void setID(NLID::DesignObjectID id) = 0;
    boost::intrusive::set_member_hook<> designNetsHook_ {};
};

}} // namespace NL // namespace naja

#endif // __SNL_NET_H_
