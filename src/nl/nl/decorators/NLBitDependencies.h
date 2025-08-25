// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "NLID.h"
#include <cstdint>

namespace naja {
namespace NL {

class NLBitDependencies {
 public:
  // Decode 64-bit blocks back into a list of bit positions
  static std::vector<size_t> decodeBits(const std::vector<uint64_t>& blocks);

  static uint64_t count_bits(uint64_t x);

  static size_t count_bits_for_vector(std::vector<uint64_t> blocks);
};

}  // namespace NL

}  // namespace naja