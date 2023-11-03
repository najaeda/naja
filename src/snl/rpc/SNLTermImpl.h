// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_TERM_IMPL_H_
#define __SNL_TERM_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLNetComponent;
  class SNLTerm;
}

class SNLTermImpl: virtual public SNLTerm::Server {
  public:
    SNLTermImpl() = default;
    SNLTermImpl(const SNLTermImpl&) = delete;
    virtual const naja::SNL::SNLNetComponent* getObject() const =0;
    virtual const naja::SNL::SNLTerm* getTerm() const=0;
    
    NAME_DECLARATION_METHOD
    SNLDESIGNOBJECT_DECLARATION_METHODS
    REFERENCE_DECLARATION_METHOD
    SIMPLE_GET_DECLARATION_METHOD(Direction);
};

#endif /* __SNL_TERM_IMPL_H */
