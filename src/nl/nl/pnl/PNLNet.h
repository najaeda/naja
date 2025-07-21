// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_NET_H_
#define __PNL_NET_H_

#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "PNLDesignObject.h"

namespace naja { namespace NL {

class PNLBitNet;

class PNLNet: public PNLDesignObject {
  public:
    friend class PNLDesign;
    using super = PNLDesignObject;

    class Type {
      public:
        enum TypeEnum {
          Undefined=0, Logical=1, Clock=2, VDD=3, GND=4, Blockage=5
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
        constexpr bool isLogical() const { return typeEnum_ == Logical; }
        constexpr bool isClock() const { return typeEnum_ == Clock; }
        constexpr bool isVDD() const { return typeEnum_ == VDD; }
        constexpr bool isGND() const { return typeEnum_ == GND; }
        constexpr bool isBlockage() const { return typeEnum_ == Blockage; }
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };

    NLID::DesignObjectReference getReference() const;
    /// \return this PNLNet unique ID in parent PNLDesign.
    virtual NLID::DesignObjectID getID() const = 0;

    /// \return net NLName.
    virtual NLName getName() const = 0;
    
    /// \return net width, 1 for PNLScalarNet and PNLBusNetBit.
    virtual NLID::Bit getWidth() const = 0;

    /**
     * \return this PNLNet PNLBitNet collection.
     * 
     * Will return itself for PNLScalarNet and PNLBusNetBit and a collection
     * of PNLBusNetBit for PNLBusNet.
     */ 
    virtual NajaCollection<PNLBitNet*> getBits() const = 0;

    /// \brief Change this PNLNet type. 
    virtual void setType(const Type& type) = 0;

    /// \return true if all bits of this net are assigned to 1'b0.
    virtual bool isGND() const = 0;
    /// \return true if all bits of this net are assigned to 1'b1.
    virtual bool isVDD() const = 0;
    /// \return true if all bits of this net are assigned to 1'b0 or 1'b1.
    bool isConstant() const { return isGND() or isVDD(); }


    //virtual bool deepCompare(const PNLNet* other, std::string& reason) const = 0;

  protected:
    PNLNet() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    virtual void destroyFromDesign() = 0;
    //virtual PNLNet* clone(PNLDesign* design) const = 0;

    //following used in BusNet and ScalarNet
    virtual void setID(NLID::DesignObjectID id) = 0;
    boost::intrusive::set_member_hook<> designNetsHook_ {};
};

}} // namespace NL // namespace naja

#endif // __PNL_NET_H_