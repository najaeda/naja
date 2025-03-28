// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_BIT_NET_H_
#define __PNL_BIT_NET_H_

#include "PNLNet.h"
#include "PNLNetComponent.h"
#include "NajaCollection.h"

namespace naja { namespace NL {

class PNLInstTerm;
class PNLBitTerm;

class PNLBitNet: public PNLNet {
  public:
    friend class PNLInstTerm;
    friend class PNLBitTerm;
    using super = PNLNet;
    
    void setType(const Type& type) override { type_ = type; }
    ///\return this PNLBitNet Type.
    Type getType() const { return type_; }
    NLID::Bit getWidth() const override { return 1; }

    bool isAssign0() const override { return type_.isAssign0(); } 
    bool isAssign1() const override { return type_.isAssign1(); } 
    bool isSupply0() const override { return type_.isSupply0(); } 
    bool isSupply1() const override { return type_.isSupply1(); } 

    ///\return the collection of PNLComponent ot this PNLBitNet
    NajaCollection<PNLNetComponent*> getComponents() const;
    ///\return the collection of PNLInstTerm of this PNLDesign (PNLInstTerm subset of getComponents())
    NajaCollection<PNLInstTerm*> getInstTerms() const;
    ///\return the collection of PNLBitTerm of this PNLDesign (PNLBitTerm subset of getComponents())
    NajaCollection<PNLBitTerm*> getBitTerms() const;

    //void connectAllComponentsTo(PNLBitNet* net);

  protected:
    PNLBitNet() = default;
    static void preCreate();
    void postCreate();
    void preDestroy() override;
    void cloneComponents(PNLBitNet* newNet) const;

  private:
    void addComponent(PNLNetComponent* net);
    void removeComponent(PNLNetComponent* net);

    using PNLBitNetComponentsHook =
      boost::intrusive::member_hook<PNLNetComponent, boost::intrusive::set_member_hook<>, &PNLNetComponent::netComponentsHook_>;
    using PNLBitNetComponents = boost::intrusive::set<PNLNetComponent, PNLBitNetComponentsHook>;

    Type                type_       { Type::Standard };
    PNLBitNetComponents components_ {};
};

}} // namespace NL // namespace naja

#endif // __PNL_BIT_NET_H_
