// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INSTTERM_IMPL_H_
#define __SNL_INSTTERM_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLNetComponentImpl.h"
#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLInstTerm;
}

class SNLInstTermImpl final: public SNLNetComponentImpl, public SNLInstTerm::Server {
  public:
    SNLInstTermImpl() = delete;
    SNLInstTermImpl(const SNLInstTermImpl&) = delete;
    SNLInstTermImpl(const naja::SNL::SNLInstTerm* instTerm);
    const naja::SNL::SNLInstTerm* getInstTerm() const {return object_; }
    const naja::SNL::SNLNetComponent* getObject() const override;
    kj::Promise<void> getInstance(GetInstanceContext context) override;
    kj::Promise<void> getTerm(GetTermContext context) override;
  private:
    const naja::SNL::SNLInstTerm* object_;
};

#endif /* __SNL_INSTTERM_IMPL_H_ */
