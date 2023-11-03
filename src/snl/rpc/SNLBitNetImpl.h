// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BIT_NET_IMPL_H_
#define __SNL_BIT_NET_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLBitNet;
}

class SNLBitNetImpl: virtual public SNLBitNet::Server {
  public:
    SNLBitNetImpl() = delete;
    SNLBitNetImpl(const SNLBitNetImpl&) = delete;
    SNLBitNetImpl(const naja::SNL::SNLBitNet* net): object_(net) {}
    const naja::SNL::SNLBitNet* getObject() const { return object_; }
    
    NAME_DECLARATION_METHOD
    SNLDESIGNOBJECT_DECLARATION_METHODS
    REFERENCE_DECLARATION_METHOD

    GET_OBJECTS_DECLARATION_METHOD(Components)

  private:
    const naja::SNL::SNLBitNet* object_;
};

#endif /* __SNL_BIT_NET_IMPL_H */
