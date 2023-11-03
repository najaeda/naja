// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_IMPL_H_
#define __SNL_DESIGN_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLDesign;
}

class SNLDesignImpl final: public SNLDesign::Server {
  public:
    SNLDesignImpl() = delete;
    SNLDesignImpl(const SNLDesignImpl&) = delete;
    SNLDesignImpl(const naja::SNL::SNLDesign* design);
    const naja::SNL::SNLDesign* getObject() const { return object_; }
    
    SNLIDOBJECT_DECLARATION_METHODS
    NAME_DECLARATION_METHOD
    kj::Promise<void> getReference(GetReferenceContext context) override;
    GET_OBJECTS_DECLARATION_METHOD(Terms)
    GET_OBJECTS_DECLARATION_METHOD(Nets)
    GET_OBJECTS_DECLARATION_METHOD(Instances)
    GET_OBJECTS_DECLARATION_METHOD(NonPrimitiveInstances)
    GET_OBJECTS_DECLARATION_METHOD(PrimitiveInstances)

  private:
    const naja::SNL::SNLDesign* object_;
};

#endif /* __SNL_DESIGN_IMPL_H */
