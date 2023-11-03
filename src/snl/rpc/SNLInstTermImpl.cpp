// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLInstTermImpl.h"

#include "SNLInstTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"

#include "SNLInstanceImpl.h"
#include "SNLScalarTermImpl.h"
#include "SNLBusTermBitImpl.h"

SNLInstTermImpl::SNLInstTermImpl(const naja::SNL::SNLInstTerm* instTerm):
  object_(instTerm)
{}

const naja::SNL::SNLNetComponent* SNLInstTermImpl::getObject() const {
  return getInstTerm();
}

kj::Promise<void> SNLInstTermImpl::getInstance(GetInstanceContext context) {
  context.getResults().setInstance(kj::heap<SNLInstanceImpl>(getInstTerm()->getInstance()));
  return kj::READY_NOW;
}

kj::Promise<void> SNLInstTermImpl::getTerm(GetTermContext context) {
  auto term = getInstTerm()->getTerm();
  if (term) {
    if (auto scalarTerm = dynamic_cast<naja::SNL::SNLScalarTerm*>(term)) {
      context.getResults().setTerm(kj::heap<SNLScalarTermImpl>(scalarTerm));
    } else if (auto busTermBit = dynamic_cast<naja::SNL::SNLBusTermBit*>(term)) {
      context.getResults().setTerm(kj::heap<SNLBusTermBitImpl>(busTermBit));
    }
  }
  return kj::READY_NOW;
}