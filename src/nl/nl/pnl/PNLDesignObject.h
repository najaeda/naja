// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLObject.h"

namespace naja { namespace NL {

class PNLDesign;

class PNLDesignObject: public NLObject {
    public:
      virtual PNLDesign* getDesign() const = 0;
};

}} // namespace NL // namespace naja