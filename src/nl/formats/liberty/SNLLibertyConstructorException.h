// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NLException.h"

namespace naja::NL {

struct SNLLibertyConstructorException: public NLException {
  public:
    SNLLibertyConstructorException() = delete;
    SNLLibertyConstructorException(const SNLLibertyConstructorException&) = default;

    SNLLibertyConstructorException(const std::string& reason):
      NLException(reason)
    {}
};

}  // namespace naja::NL