// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NLException.h"

namespace naja::NL {

struct SNLDumpException: public NLException {
  public:
    SNLDumpException() = delete;
    SNLDumpException(const SNLDumpException&) = default;

    SNLDumpException(const std::string& reason): NLException(reason)
    {}
};

}  // namespace naja::NL