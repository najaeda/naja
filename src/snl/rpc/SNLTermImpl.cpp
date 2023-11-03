/*
 * Copyright 2022 The Naja Authors.
 */

#include "SNLTermImpl.h"

#include <capnp/message.h>

#include "SNLTerm.h"

#include "SNLDesignImpl.h"
#include "SNLRPCImplementationMacros.h"

SNLDESIGNOBJECT_METHODS(SNLTermImpl)
NAME_METHOD(SNLTermImpl, getTerm())
REFERENCE_METHOD(SNLTermImpl, getTerm())

kj::Promise<void> SNLTermImpl::getDirection(GetDirectionContext context) {
  if (getTerm()) {
    switch (getTerm()->getDirection()) {
      case naja::SNL::SNLTerm::Direction::Input:
        context.getResults().setDirection(Direction::INPUT);
        break;
      case naja::SNL::SNLTerm::Direction::Output:
        context.getResults().setDirection(Direction::OUTPUT);
        break;
      case naja::SNL::SNLTerm::Direction::InOut:
        context.getResults().setDirection(Direction::INOUT);
        break;
    }
  }
  return kj::READY_NOW;
}