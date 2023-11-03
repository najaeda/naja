// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_RPC_EXCEPTION_H_
#define __SNL_RPC_EXCEPTION_H_

#include "NajaException.h"

struct SNLRPCException: public naja::NajaException {
  public:
    SNLRPCException() = delete;
    SNLRPCException(const SNLRPCException&) = default;

    SNLRPCException(const std::string& reason):
      NajaException(reason)
    {}
};

#endif // __SNL_RPC_EXCEPTION_H_
