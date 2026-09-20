// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

namespace naja::NL {

class SNLBitNet;
class SNLDesign;
class SNLInstance;
class SNLNet;

/// Hardware construction after language-specific sizing and process analysis.
/// Inputs must belong to design. Invalid inputs throw NLException before an
/// instance is created. Source links and initialization remain caller-owned.
class SNLRTLPrimitives {
  public:
    using Bits = std::vector<SNLBitNet*>;

    /// Input vectors are least-significant-bit first. Select 0 chooses a;
    /// select 1 chooses b. Output is connected by its declared bit order.
    static SNLInstance* createMux(
      SNLDesign* design, SNLBitNet* select,
      const Bits& a, const Bits& b, SNLNet* output);

    /// One positive-edge register without reset or enable. All nets have width 1.
    static SNLInstance* createDFF(
      SNLDesign* design, SNLNet* clock, SNLNet* data, SNLNet* output);
};

}  // namespace naja::NL
