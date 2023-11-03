// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BUSTERM_BIT_IMPL_H_
#define __SNL_BUSTERM_BIT_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLBitTermImpl.h"
#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLBusTermBit;
}

class SNLBusTermBitImpl final:
  public SNLBitTermImpl,
  public SNLBusTermBit::Server {
  public:
    SNLBusTermBitImpl() = delete;
    SNLBusTermBitImpl(const SNLBusTermBitImpl&) = delete;
    SNLBusTermBitImpl(const naja::SNL::SNLBusTermBit* bit);
    const naja::SNL::SNLBusTermBit* getBusTermBit() const { return object_; }
    const naja::SNL::SNLNetComponent* getObject() const override;
    
    SIMPLE_GET_DECLARATION_METHOD(Bit)
  private:
    const naja::SNL::SNLBusTermBit* object_;
};

#endif /* __SNL_BUSTERM_BIT_IMPL_H */
