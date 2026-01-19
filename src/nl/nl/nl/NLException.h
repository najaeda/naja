// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NajaException.h"

namespace naja::NL {

struct NLException: public NajaException {
  public:
    NLException() = delete;
    NLException(const NLException&) = default;

    NLException(const std::string& reason):
      NajaException(std::move(reason))
    {}
};

}  // namespace naja::NL