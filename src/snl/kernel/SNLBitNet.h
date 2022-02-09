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

#ifndef __SNL_BIT_NET_H_
#define __SNL_BIT_NET_H_

#include "SNLNet.h"
#include "SNLNetComponent.h"
#include "SNLCollection.h"

namespace naja { namespace SNL {

class SNLInstTerm;
class SNLBitTerm;

class SNLBitNet: public SNLNet {
  public:
    friend class SNLNetComponent;
    using super = SNLNet;
    
    void setType(const Type& type) override { type_ = type; }
    Type getType() const { return type_; }

    ///\return the collection of SNLComponent ot this SNLBitNet
    SNLCollection<SNLNetComponent*> getComponents() const;
    ///\return the collection of SNLInstTerm of this SNLDesign (SNLInstTerm subset of getComponents())
    SNLCollection<SNLInstTerm*> getInstTerms() const;
    ///\return the collection of SNLBitTerm of this SNLDesign (SNLBitTerm subset of getComponents())
    SNLCollection<SNLBitTerm*> getBitTerms() const;

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