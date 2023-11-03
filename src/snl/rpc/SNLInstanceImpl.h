// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INSTANCE_IMPL_H_
#define __SNL_INSTANCE_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLInstance;
}

class SNLInstanceImpl final: public SNLInstance::Server {
  public:
    SNLInstanceImpl() = delete;
    SNLInstanceImpl(const SNLInstanceImpl&) = delete;
    SNLInstanceImpl(const naja::SNL::SNLInstance* instance): object_(instance) {}
    
    NAME_DECLARATION_METHOD
    SNLDESIGNOBJECT_DECLARATION_METHODS
    REFERENCE_DECLARATION_METHOD
    kj::Promise<void> getModel(GetModelContext context) override;
    const naja::SNL::SNLInstance* getObject() const { return object_; }
    GET_OBJECTS_DECLARATION_METHOD(InstTerms)

  private:
    const naja::SNL::SNLInstance* object_;
};

#endif /* __SNL_INSTANCE_IMPL_H */
