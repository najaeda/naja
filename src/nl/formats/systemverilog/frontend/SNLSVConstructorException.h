// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "NLException.h"

namespace naja::NL {

struct SNLSVConstructorException: public NLException {
  public:
    SNLSVConstructorException() = delete;
    SNLSVConstructorException(const SNLSVConstructorException&) = default;

    SNLSVConstructorException(const std::string& reason):
      NLException(reason)
    {}
};

}  // namespace naja::NL
