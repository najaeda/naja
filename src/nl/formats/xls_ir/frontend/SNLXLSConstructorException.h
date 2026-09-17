// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#include "NLException.h"

namespace naja::NL {

struct SNLXLSConstructorException: public NLException {
  SNLXLSConstructorException() = delete;
  SNLXLSConstructorException(const SNLXLSConstructorException&) = default;

  explicit SNLXLSConstructorException(const std::string& reason):
    NLException(reason) {}
};

}  // namespace naja::NL
