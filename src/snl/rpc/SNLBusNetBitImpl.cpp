// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusNetBitImpl.h"

#include <capnp/message.h>

#include "SNLBusNetBit.h"

#include "SNLRPCImplementationMacros.h"

SNLBusNetBitImpl::SNLBusNetBitImpl(const naja::SNL::SNLBusNetBit* bit):
  SNLBitNetImpl(bit)
{}

const naja::SNL::SNLBusNetBit* SNLBusNetBitImpl::getBusNetBit() const {
  return static_cast<const naja::SNL::SNLBusNetBit*>(getObject());
}

SIMPLE_GET_METHOD(SNLBusNetBitImpl, getBusNetBit(), Bit, Bit)