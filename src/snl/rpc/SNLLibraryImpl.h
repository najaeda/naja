// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_LIBRARY_IMPL_H_
#define __SNL_LIBRARY_IMPL_H_

#include "snl_rpc.capnp.h"

#include "SNLRPCDeclarationMacros.h"

namespace naja::SNL {
  class SNLLibrary;
}

class SNLLibraryImpl final: public SNLLibrary::Server {
  public:
    SNLLibraryImpl() = delete;
    SNLLibraryImpl(const SNLLibraryImpl&) = delete;
    SNLLibraryImpl(const naja::SNL::SNLLibrary* library): object_(library) {}
    const naja::SNL::SNLLibrary* getObject() const { return object_; }

    SNLIDOBJECT_DECLARATION_METHODS
    NAME_DECLARATION_METHOD
    GET_OBJECTS_DECLARATION_METHOD(Designs)

  private:
    const naja::SNL::SNLLibrary* object_;
};

#endif /* __SNL_LIBRARY_IMPL_H */
