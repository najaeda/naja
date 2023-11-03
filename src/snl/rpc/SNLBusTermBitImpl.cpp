// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusTermBitImpl.h"

#include <capnp/message.h>

#include "SNLBusTermBit.h"

#include "SNLRPCImplementationMacros.h"

SNLBusTermBitImpl::SNLBusTermBitImpl(const naja::SNL::SNLBusTermBit* bit):
  object_(bit)
{}

const naja::SNL::SNLNetComponent* SNLBusTermBitImpl::getObject() const {
  return getBusTermBit();
}

SIMPLE_GET_METHOD(SNLBusTermBitImpl, getBusTermBit(), Bit, Bit)