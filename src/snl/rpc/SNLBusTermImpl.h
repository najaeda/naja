// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BUSTERM_IMPL_H_
#define __SNL_BUSTERM_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLTermImpl.h"
#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLBusTerm;
}

class SNLBusTermImpl final: public SNLTermImpl, SNLBusTerm::Server {
  public:
    //using SNLBusTerm::Server::dispatchCall;
    SNLBusTermImpl() = delete;
    SNLBusTermImpl(const SNLBusTermImpl&) = delete;
    SNLBusTermImpl(const naja::SNL::SNLBusTerm* term);
    const naja::SNL::SNLNetComponent* getObject() const override;
    const naja::SNL::SNLTerm* getTerm() const override;
    const naja::SNL::SNLBusTerm* getBusTerm() const { return object_; }
    
    GET_OBJECTS_DECLARATION_METHOD(BusBits)
    SIMPLE_GET_DECLARATION_METHOD(MSB)
    SIMPLE_GET_DECLARATION_METHOD(LSB)
    SIMPLE_GET_DECLARATION_METHOD(MSBLSB)

  private:
    const naja::SNL::SNLBusTerm* object_;
};

#endif /* __SNL_BUSTERM_IMPL_H */
