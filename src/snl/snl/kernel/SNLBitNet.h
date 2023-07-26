// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
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
    Type getType() const { return type_; }
    SNLID::Bit getSize() const override { return 1; }

    bool isConstant0() const { return type_.isConst0(); }
    bool isConstant1() const { return type_.isConst1(); }
    bool isAssignConstant() const override { return type_.isAssign(); } 

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
