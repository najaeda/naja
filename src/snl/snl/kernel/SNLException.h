// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_EXCEPTION_H_
#define __SNL_EXCEPTION_H_

#include "NajaException.h"

namespace naja { namespace SNL {

struct SNLException: public NajaException {
  public:
    SNLException() = delete;
    SNLException(const SNLException&) = default;

    SNLException(const std::string& reason):
      NajaException(reason)
    {}
};

}} // namespace SNL // namespace naja

#endif // __SNL_EXCEPTION_H_
