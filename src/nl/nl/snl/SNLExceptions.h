// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLException.h"

namespace naja::NL {

class SNLDesignNameConflictException : public NLException {
  public:
    SNLDesignNameConflictException(const std::string& reason):
      NLException(reason)
    {}
};

}