/*
 * Copyright 2022 The Naja Authors.
 */

#include "SNLScalarTermImpl.h"

#include <capnp/message.h>

#include "SNLScalarTerm.h"
//#include "SNLRPCImplementationMacros.h"

SNLScalarTermImpl::SNLScalarTermImpl(const naja::SNL::SNLScalarTerm* term):
  object_(term)
{}

const naja::SNL::SNLTerm* SNLScalarTermImpl::getTerm() const {
  return getScalarTerm();
}

const naja::SNL::SNLNetComponent* SNLScalarTermImpl::getObject() const {
  return getScalarTerm();
}
