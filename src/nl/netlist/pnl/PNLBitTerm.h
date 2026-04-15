// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PNLTerm.h"

namespace naja::NL {

class PNLBitTerm: public PNLTerm {
  public:
    friend class PNLBitNet;
    using super = PNLTerm;
    
    //Allows to compare PNLBitTerms in the same design.
    struct InDesignLess {
      bool operator()(const PNLBitTerm* lt, const PNLBitTerm* rt) const {
        return std::make_pair(lt->getID(), lt->getBit()) < std::make_pair(rt->getID(), rt->getBit());
      }
    };

    NLID::Bit getWidth() const override { return 1; }
    virtual NLID::Bit getBit() const = 0;
    PNLBitNet* getNet() const override { return net_; }
    void setNet(PNLNet* net) override;
    NLID::DesignObjectID getOrderID() const { return orderID_; }
    void setOrderID(NLID::DesignObjectID orderID) { orderID_ = orderID; }
    
  protected:
    PNLBitTerm() = default;
    static void preCreate();
    void postCreate() override;
    void preDestroy() override;
  private:
    PNLBitNet*            net_      { nullptr };
    NLID::DesignObjectID  orderID_  { (NLID::DesignObjectID) -1 };
};

}  // namespace naja::NL