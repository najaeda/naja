// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BIT_NET_H_
#define __SNL_BIT_NET_H_

#include "SNLNet.h"
#include "SNLNetComponent.h"
#include "NajaCollection.h"

namespace naja { namespace SNL {

class SNLInstTerm;
class SNLBitTerm;

class SNLBitNet: public SNLNet {
  public:
    friend class SNLInstTerm;
    friend class SNLBitTerm;
    using super = SNLNet;
    
    void setType(const Type& type) override { type_ = type; }
    ///\return this SNLBitNet Type.
    Type getType() const { return type_; }
    SNLID::Bit getWidth() const override { return 1; }

    bool isAssign0() const override { return type_.isAssign0(); } 
    bool isAssign1() const override { return type_.isAssign1(); } 
    bool isSupply0() const override { return type_.isSupply0(); } 
    bool isSupply1() const override { return type_.isSupply1(); } 

    ///\return the collection of SNLComponent ot this SNLBitNet
    NajaCollection<SNLNetComponent*> getComponents() const;
    ///\return the collection of SNLInstTerm of this SNLDesign (SNLInstTerm subset of getComponents())
    NajaCollection<SNLInstTerm*> getInstTerms() const;
    ///\return the collection of SNLBitTerm of this SNLDesign (SNLBitTerm subset of getComponents())
    NajaCollection<SNLBitTerm*> getBitTerms() const;

    void connectAllComponentsTo(SNLBitNet* net);

  protected:
    SNLBitNet() = default;
    static void preCreate();
    void postCreate();
    void preDestroy() override;
    void cloneComponents(SNLBitNet* newNet) const;

  private:
    void addComponent(SNLNetComponent* net);
    void removeComponent(SNLNetComponent* net);

    using SNLBitNetComponentsHook =
      boost::intrusive::member_hook<SNLNetComponent, boost::intrusive::set_member_hook<>, &SNLNetComponent::netComponentsHook_>;
    using SNLBitNetComponents = boost::intrusive::set<SNLNetComponent, SNLBitNetComponentsHook>;

    Type                type_       { Type::Standard };
    SNLBitNetComponents components_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_BIT_NET_H_
