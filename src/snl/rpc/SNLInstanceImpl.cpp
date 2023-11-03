// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLInstanceImpl.h"

#include <capnp/message.h>

#include "SNLInstance.h"

#include "SNLRPCImplementationMacros.h"
#include "SNLDesignImpl.h"
#include "SNLInstTermImpl.h"

SNLDESIGNOBJECT_METHODS(SNLInstanceImpl);
NAME_METHOD(SNLInstanceImpl, getObject());
REFERENCE_METHOD(SNLInstanceImpl, getObject())
GET_OBJECTS_METHOD(SNLInstanceImpl, getObject(), InstTerm, InstTerms);

kj::Promise<void> SNLInstanceImpl::getModel(GetModelContext context) {
  if (object_) {
    context.getResults().setModel(kj::heap<SNLDesignImpl>(object_->getModel()));
  }
  return kj::READY_NOW;
}
