// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BIT_TERM_IMPL_H_
#define __SNL_BIT_TERM_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLNetComponentImpl.h"
#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLBitTerm;
}

class SNLBitTermImpl:
  public virtual SNLNetComponentImpl,
  public virtual SNLBitTerm::Server {
  public:
    SNLBitTermImpl() = default;
    SNLBitTermImpl(const SNLBitTermImpl&) = delete;
};

#endif /* __SNL_BIT_TERM_IMPL_H_ */