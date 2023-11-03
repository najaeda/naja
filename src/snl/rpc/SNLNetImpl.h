// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_NET_IMPL_H_
#define __SNL_NET_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLNet;
}

class SNLNetImpl: virtual public SNLNet::Server {
  public:
    SNLNetImpl() = delete;
    SNLNetImpl(const SNLNetImpl&) = delete;
    SNLNetImpl(const naja::SNL::SNLNet* net): object_(net) {}
    const naja::SNL::SNLNet* getObject() const { return object_; }
    
    NAME_DECLARATION_METHOD
    SNLDESIGNOBJECT_DECLARATION_METHODS
    REFERENCE_DECLARATION_METHOD

  private:
    const naja::SNL::SNLNet* object_;
};

#endif /* __SNL_NET_IMPL_H */
