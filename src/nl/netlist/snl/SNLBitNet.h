// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "SNLNet.h"
#include "SNLNetComponent.h"
#include "NajaCollection.h"

namespace naja::NL {

class SNLInstTerm;
class SNLBitTerm;

class SNLBitNet: public SNLNet {
  public:
    friend class SNLInstTerm;
    friend class SNLBitTerm;
    using super = SNLNet;
    
    NLID::Bit getWidth() const override { return 1; }

    ///\return the collection of SNLComponent ot this SNLBitNet
    NajaCollection<SNLNetComponent*> getComponents() const;
    ///\return the collection of SNLInstTerm of this SNLDesign (SNLInstTerm subset of getComponents())
    NajaCollection<SNLInstTerm*> getInstTerms() const;
    ///\return the collection of SNLBitTerm of this SNLDesign (SNLBitTerm subset of getComponents())
    NajaCollection<SNLBitTerm*> getBitTerms() const;

    void connectAllComponentsTo(SNLBitNet* net);

    bool compareComponents(const SNLBitNet* otherNet, std::string& reason) const;

  protected:
    SNLBitNet() = default;
    static void preCreate();
    void postCreate() override;
    void preDestroy() override;
    void cloneComponents(SNLBitNet* newNet) const;

  private:
    void addComponent(SNLNetComponent* net);
    void removeComponent(SNLNetComponent* net);

    using SNLBitNetComponentsHook =
      boost::intrusive::member_hook<SNLNetComponent, boost::intrusive::set_member_hook<>, &SNLNetComponent::netComponentsHook_>;
    using SNLBitNetComponents = boost::intrusive::set<SNLNetComponent, SNLBitNetComponentsHook>;

    SNLBitNetComponents components_ {};
};

}  // namespace naja::NL
