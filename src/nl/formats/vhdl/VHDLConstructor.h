// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLLibrary.h"

#include <string_view>

namespace naja::NL {

class SNLDesign;

/// Temporary Naja adapter for the scalar mux and clocked-register proof.
/// The standalone VHDL frontend remains independent of this class and SNL.
class VHDLConstructor {
  public:
    explicit VHDLConstructor(NLLibrary* library) : library_(library) {}

    /// Parse and lower one entity/architecture with one scalar conditional
    /// assignment or one positive-edge event-guarded process over bit ports
    /// and internal signals, with one scheduled write per destination.
    SNLDesign* construct(std::string_view source) const;

  private:
    NLLibrary* library_;
};

}  // namespace naja::NL
