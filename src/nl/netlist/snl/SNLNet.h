// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "SNLDesignObject.h"

namespace naja::NL {

class SNLBitNet;

class SNLNet: public SNLDesignObject {
  public:
    friend class SNLDesign;
    using super = SNLDesignObject;

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

}  // namespace naja::NL
