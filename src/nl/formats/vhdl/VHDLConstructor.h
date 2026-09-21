// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLLibrary.h"

#include <string_view>

namespace naja::NL {

class SNLDesign;

/// Temporary Naja adapter for the first scalar conditional-assignment proof.
/// The standalone VHDL frontend remains independent of this class and SNL.
class VHDLConstructor {
  public:
    explicit VHDLConstructor(NLLibrary* library) : library_(library) {}

    /// Parse and lower one entity/architecture with one scalar conditional
    /// assignment of the form y <= a when sel = '1' else b.
    SNLDesign* construct(std::string_view source) const;

  private:
    NLLibrary* library_;
};

}  // namespace naja::NL
