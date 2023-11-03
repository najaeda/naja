// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#include "SNLBusTermImpl.h"

#include <capnp/message.h>

#include "SNLBusTerm.h"

#include "SNLDesignImpl.h"
#include "SNLBusTermBitImpl.h"
#include "SNLRPCImplementationMacros.h"

SNLBusTermImpl::SNLBusTermImpl(const naja::SNL::SNLBusTerm* term):
  object_(term)
{}
const naja::SNL::SNLNetComponent* SNLBusTermImpl::getObject() const {
  return getObject();
}

const naja::SNL::SNLTerm* SNLBusTermImpl::getTerm() const {
  return getBusTerm();
}

GET_OBJECTS_METHOD(SNLBusTermImpl, getBusTerm(), BusTermBit, BusBits)
SIMPLE_GET_METHOD(SNLBusTermImpl, getBusTerm(), MSB, Msb)
SIMPLE_GET_METHOD(SNLBusTermImpl, getBusTerm(), LSB, Lsb)

kj::Promise<void> SNLBusTermImpl::getMSBLSB(GetMSBLSBContext context) {
  if (getBusTerm()) {
    context.getResults().setMsb(getBusTerm()->getMSB());
    context.getResults().setLsb(getBusTerm()->getLSB());
  }
  return kj::READY_NOW;
}
