// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BUSNET_BIT_IMPL_H_
#define __SNL_BUSNET_BIT_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLBitNetImpl.h"
#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLBusNetBit;
}

class SNLBusNetBitImpl final: public SNLBitNetImpl, public SNLBusNetBit::Server {
  public:
    SNLBusNetBitImpl() = delete;
    SNLBusNetBitImpl(const SNLBusNetBitImpl&) = delete;
    SNLBusNetBitImpl(const naja::SNL::SNLBusNetBit* bit);
    const naja::SNL::SNLBusNetBit* getBusNetBit() const;
    
    SIMPLE_GET_DECLARATION_METHOD(Bit)
};

#endif /* __SNL_BUSNET_BIT_IMPL_H */
