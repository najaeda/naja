// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __SNL_DUMP_EXCEPTION_H_
#define __SNL_DUMP_EXCEPTION_H_

#include "SNLException.h"

namespace naja { namespace SNL {

struct SNLDumpException: public SNLException {
  public:
    SNLDumpException() = delete;
    SNLDumpException(const SNLDumpException&) = default;

    SNLDumpException(const std::string& reason): SNLException(reason)
    {}
};

}} // namespace SNL // namespace naja

#endif // __SNL_DUMP_EXCEPTION_H_
