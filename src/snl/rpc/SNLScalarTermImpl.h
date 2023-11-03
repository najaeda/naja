// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_SCALAR_TERM_IMPL_H_
#define __SNL_SCALAR_TERM_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLBitTermImpl.h"
#include "SNLTermImpl.h"
#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLScalarTerm;
}

class SNLScalarTermImpl final: 
  public SNLBitTermImpl,
  public SNLTermImpl,
  public SNLScalarTerm::Server {
  public:
    SNLScalarTermImpl() = delete;
    SNLScalarTermImpl(const SNLScalarTermImpl&) = delete;
    SNLScalarTermImpl(const naja::SNL::SNLScalarTerm* term);
    const naja::SNL::SNLNetComponent* getObject() const override;
    const naja::SNL::SNLTerm* getTerm() const override;
    const naja::SNL::SNLScalarTerm* getScalarTerm() const { return object_; }
  private:
    const naja::SNL::SNLScalarTerm* object_;
};

#endif /* __SNL_SCALAR_TERM_IMPL_H_ */
