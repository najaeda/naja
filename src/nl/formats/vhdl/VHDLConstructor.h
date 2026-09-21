// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLLibrary.h"

#include <string_view>

namespace naja::NL {

class SNLDesign;

/// Temporary Naja adapter for scalar combinational and clocked-register proofs.
/// The standalone VHDL frontend remains independent of this class and SNL.
class VHDLConstructor {
  public:
    explicit VHDLConstructor(NLLibrary* library) : library_(library) {}

    /// Parse and lower one entity/architecture with one scalar bit expression,
    /// including logical gates and a conditional assignment, or one positive-edge
    /// event-guarded process over bit ports and internal signals, with one
    /// scheduled write per signal destination.
    /// Scalar process variables may be temporary or retained state; retained
    /// variables must be assigned on every activation.
    SNLDesign* construct(std::string_view source) const;

  private:
    NLLibrary* library_;
};

}  // namespace naja::NL
