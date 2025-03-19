// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DUMP_EXCEPTION_H_
#define __SNL_DUMP_EXCEPTION_H_

#include "NLException.h"

namespace naja { namespace SNL {

struct SNLDumpException: public NLException {
  public:
    SNLDumpException() = delete;
    SNLDumpException(const SNLDumpException&) = default;

    SNLDumpException(const std::string& reason): NLException(reason)
    {}
};

}} // namespace SNL // namespace naja

#endif // __SNL_DUMP_EXCEPTION_H_
