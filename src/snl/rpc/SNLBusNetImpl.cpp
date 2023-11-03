// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusNetImpl.h"

#include <capnp/message.h>

#include "SNLBusNet.h"

#include "SNLDesignImpl.h"
#include "SNLBusNetBitImpl.h"
#include "SNLRPCImplementationMacros.h"

SNLBusNetImpl::SNLBusNetImpl(const naja::SNL::SNLBusNet* net):
  SNLNetImpl(net)
{}

GET_OBJECTS_METHOD(SNLBusNetImpl, getBusNet(), BusNetBit, BusBits)
SIMPLE_GET_METHOD(SNLBusNetImpl, getBusNet(), MSB, Msb)
SIMPLE_GET_METHOD(SNLBusNetImpl, getBusNet(), LSB, Lsb)

const naja::SNL::SNLBusNet* SNLBusNetImpl::getBusNet() const {
  return static_cast<const naja::SNL::SNLBusNet*>(getObject());
}

kj::Promise<void> SNLBusNetImpl::getMSBLSB(GetMSBLSBContext context) {
  if (getBusNet()) {
    context.getResults().setMsb(getBusNet()->getMSB());
    context.getResults().setLsb(getBusNet()->getLSB());
  }
  return kj::READY_NOW;
}
