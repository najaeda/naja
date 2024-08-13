// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_LIBERTY_CONSTRUCTOR_EXCEPTION_H_
#define __SNL_LIBERTY_CONSTRUCTOR_EXCEPTION_H_

#include "SNLException.h"

namespace naja { namespace SNL {

struct SNLLibertyConstructorException: public SNLException {
  public:
    SNLLibertyConstructorException() = delete;
    SNLLibertyConstructorException(const SNLLibertyConstructorException&) = default;

    SNLLibertyConstructorException(const std::string& reason):
      SNLException(reason)
    {}
};

}} // namespace SNL // namespace naja

#endif // __SNL_LIBERTY_CONSTRUCTOR_EXCEPTION_H_