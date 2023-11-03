// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DB_IMPL_H_
#define __SNL_DB_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLDB;
}

class SNLDBImpl final: public SNLDB::Server {
  public:
    SNLDBImpl() = delete;
    SNLDBImpl(const SNLDBImpl&) = delete;
    SNLDBImpl(const naja::SNL::SNLDB* db): object_(db) {}
    const naja::SNL::SNLDB* getObject() const { return object_; }

    SNLIDOBJECT_DECLARATION_METHODS
    GET_OBJECTS_DECLARATION_METHOD(Libraries)
    kj::Promise<void> getTopDesign(GetTopDesignContext context) override;

  private:
    const naja::SNL::SNLDB* object_;

};

#endif /* __SNL_DB_IMPL_H */
