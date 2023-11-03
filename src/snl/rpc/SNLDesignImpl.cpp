// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignImpl.h"

#include <capnp/message.h>

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"

#include "SNLRPCImplementationMacros.h"
#include "SNLScalarTermImpl.h"
#include "SNLBusTermImpl.h"
#include "SNLNetImpl.h"
#include "SNLBusNetImpl.h"
#include "SNLInstanceImpl.h"
#include "SNLRPCException.h"

SNLDesignImpl::SNLDesignImpl(const naja::SNL::SNLDesign* design): object_(design) {
  if (not object_) {
    throw SNLRPCException("Cannot reference a NULL design");
  }
}

SNLIDOBJECT_METHODS(SNLDesignImpl);
NAME_METHOD(SNLDesignImpl, getObject());
GET_BUS_OBJECTS_METHOD(SNLDesignImpl, Term, BusTerm, ScalarTerm);
GET_BUS_OBJECTS_METHOD(SNLDesignImpl, Net, BusNet, Net);
GET_OBJECTS_METHOD(SNLDesignImpl, getObject(), Instance, Instances);
GET_OBJECTS_METHOD(SNLDesignImpl, getObject(), Instance, NonPrimitiveInstances);
GET_OBJECTS_METHOD(SNLDesignImpl, getObject(), Instance, PrimitiveInstances);

kj::Promise<void> SNLDesignImpl::getReference(GetReferenceContext context) {
  ::capnp::MallocMessageBuilder message;
  SNLDesignReference::Builder reference = message.initRoot<SNLDesignReference>();
  auto snlRef = getObject()->getReference();
  reference.setDbID(snlRef.dbID_);
  reference.setLibraryID(snlRef.getDBDesignReference().libraryID_);
  reference.setDesignID(snlRef.getDBDesignReference().designID_);
  context.getResults().setReference(reference);
  return kj::READY_NOW;
}
