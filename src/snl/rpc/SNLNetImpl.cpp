/*
 * Copyright 2022 The Naja Authors.
 */

#include "SNLNetImpl.h"

#include <capnp/message.h>

#include "SNLNet.h"

#include "SNLDesignImpl.h"
#include "SNLRPCImplementationMacros.h"

SNLDESIGNOBJECT_METHODS(SNLNetImpl);
NAME_METHOD(SNLNetImpl, getObject());
REFERENCE_METHOD(SNLNetImpl, getObject())
