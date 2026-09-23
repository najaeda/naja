// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vhdl/Parser.h"

namespace naja::NL {
class NLLibrary;
class SNLDesign;

// Statically indexed, two-state RTL profile. This path performs its own
// elaboration and type checks before returning a completed design.
bool requiresVHDLRTL(const vhdl::DesignFile& syntax);
SNLDesign* constructVHDLRTL(NLLibrary* library, const vhdl::DesignFile& syntax,
                          std::string_view top, std::string_view source);
}
