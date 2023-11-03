/*
 * Copyright 2022 The Naja Authors.
 */

#include "SNLNetComponentImpl.h"

#include <capnp/message.h>

#include "SNLNetComponent.h"
#include "SNLBusNetBit.h"

#include "SNLDesignImpl.h"
#include "SNLBitNetImpl.h"
#include "SNLBusNetBitImpl.h"
#include "SNLRPCImplementationMacros.h"

SNLDESIGNOBJECT_METHODS(SNLNetComponentImpl)

kj::Promise<void> SNLNetComponentImpl::getNet(GetNetContext context) {
  ::capnp::MallocMessageBuilder message;
  BitNetInfo::Builder info = message.initRoot<BitNetInfo>();
  if (auto busBitObject = dynamic_cast<const naja::SNL::SNLBusNetBit*>(getObject())) {
    info.setNet(kj::heap<SNLBusNetBitImpl>(busBitObject));
    info.setIsBusBit(true);
  } else {
    info.setNet(kj::heap<SNLBitNetImpl>(busBitObject));
    info.setIsBusBit(false);
  }
  context.getResults().setBitNetInfo(info);
  return kj::READY_NOW;
}