// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NLException.h"

namespace naja::NL {

struct SNLVRLConstructorException: public NLException {
  public:
    SNLVRLConstructorException() = delete;
    SNLVRLConstructorException(const SNLVRLConstructorException&) = default;

    SNLVRLConstructorException(const std::string& reason):
      NLException(reason)
    {}
};

}  // namespace naja::NL