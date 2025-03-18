// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NL_EXCEPTION_H_
#define __NL_EXCEPTION_H_

#include "NajaException.h"

namespace naja { namespace SNL {

struct NLException: public NajaException {
  public:
    NLException() = delete;
    NLException(const NLException&) = default;

    NLException(const std::string& reason):
      NajaException(std::move(reason))
    {}
};

}} // namespace SNL // namespace naja

#endif // __NL_EXCEPTION_H_
