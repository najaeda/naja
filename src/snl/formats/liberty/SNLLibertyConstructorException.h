// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_LIBERTY_CONSTRUCTOR_EXCEPTION_H_
#define __SNL_LIBERTY_CONSTRUCTOR_EXCEPTION_H_

#include "NLException.h"

namespace naja { namespace SNL {

struct SNLLibertyConstructorException: public NLException {
  public:
    SNLLibertyConstructorException() = delete;
    SNLLibertyConstructorException(const SNLLibertyConstructorException&) = default;

    SNLLibertyConstructorException(const std::string& reason):
      NLException(reason)
    {}
};

}} // namespace SNL // namespace naja

#endif // __SNL_LIBERTY_CONSTRUCTOR_EXCEPTION_H_
