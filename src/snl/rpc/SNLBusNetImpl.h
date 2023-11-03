// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BUSNET_IMPL_H_
#define __SNL_BUSNET_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLNetImpl.h"
#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLBusNet;
}

class SNLBusNetImpl final: public SNLNetImpl, SNLBusNet::Server {
  public:
    SNLBusNetImpl() = delete;
    SNLBusNetImpl(const SNLBusNetImpl&) = delete;
    SNLBusNetImpl(const naja::SNL::SNLBusNet* net);
    const naja::SNL::SNLBusNet* getBusNet() const;
    
    GET_OBJECTS_DECLARATION_METHOD(BusBits)
    SIMPLE_GET_DECLARATION_METHOD(MSB)
    SIMPLE_GET_DECLARATION_METHOD(LSB)
    SIMPLE_GET_DECLARATION_METHOD(MSBLSB)
};

#endif /* __SNL_BUSNET_IMPL_H */
