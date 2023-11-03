// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_NET_COMPONENT_IMPL_H_
#define __SNL_NET_COMPONENT_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLNetComponent;
}

class SNLNetComponentImpl: virtual public SNLNetComponent::Server {
  public:
    SNLNetComponentImpl() = default;
    SNLNetComponentImpl(const SNLNetComponentImpl&) = delete;
    virtual const naja::SNL::SNLNetComponent* getObject() const =0;
    
    SNLDESIGNOBJECT_DECLARATION_METHODS
    kj::Promise<void> getNet(GetNetContext context) override;
};

#endif /* __SNL_NET_COMPONENT_IMPL_H_ */
