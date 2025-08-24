// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS> SPDX-License-Identifier:
// Apache-2.0

#pragma once

#include <vector>
#include "NLID.h"

namespace naja {
namespace NL {

class NLBitDependencies {
 public:
  // Encode a list of non-negative bit positions into 64-bit blocks.
  // Blocks with no set bits at the high end are trimmed off.
  static std::vector<uint64_t> encodeBits(
      const std::vector<size_t>& positions);

  // Decode 64-bit blocks back into a list of bit positions
  static std::vector<size_t> decodeBits(const std::vector<uint64_t>& blocks);

  static uint64_t count_bits(uint64_t x);
};

}  // namespace NL

}  // namespace naja