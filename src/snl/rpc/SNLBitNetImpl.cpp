// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBitNetImpl.h"

#include <capnp/message.h>

#include "SNLBitNet.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"

#include "SNLScalarTermImpl.h"
#include "SNLBusTermBitImpl.h"
#include "SNLInstTermImpl.h"
#include "SNLDesignImpl.h"
#include "SNLRPCImplementationMacros.h"

SNLDESIGNOBJECT_METHODS(SNLBitNetImpl)
NAME_METHOD(SNLBitNetImpl, getObject())
REFERENCE_METHOD(SNLBitNetImpl, getObject())

kj::Promise<void> SNLBitNetImpl::getComponents(GetComponentsContext context) {
  if (object_) {
    auto objects = context.getResults().initComponents(object_->getComponents().size());
    unsigned index = 0;
    for (auto snlComponent: object_->getComponents()) {
      if (auto scalarTerm = dynamic_cast<naja::SNL::SNLScalarTerm*>(snlComponent)) {
        objects.set(index++, kj::heap<SNLScalarTermImpl>(scalarTerm));
      } else if (auto busTermBit = dynamic_cast<naja::SNL::SNLBusTermBit*>(snlComponent)) {
        objects.set(index++, kj::heap<SNLBusTermBitImpl>(busTermBit));
      } else {
        auto instTerm = static_cast<naja::SNL::SNLInstTerm*>(snlComponent);
        objects.set(index++, kj::heap<SNLInstTermImpl>(instTerm));
      }
      //++index;
    }
  }
  return kj::READY_NOW;
}
