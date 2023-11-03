// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDBImpl.h"

#include <capnp/message.h>

#include "SNLDB.h"

#include "SNLRPCImplementationMacros.h"
#include "SNLLibraryImpl.h"
#include "SNLDesignImpl.h"

SNLIDOBJECT_METHODS(SNLDBImpl);
GET_OBJECTS_METHOD(SNLDBImpl, getObject(), Library, Libraries);

kj::Promise<void> SNLDBImpl::getTopDesign(GetTopDesignContext context) {
  context.getResults().setDesign(kj::heap<SNLDesignImpl>(getObject()->getTopDesign()));
  return kj::READY_NOW;
}
