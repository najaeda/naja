// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "NLID.h"
#include <cstdint>

namespace naja::NL {

class NLBitDependencies {
 public:
  // Encode a list of non-negative bit positions into 64-bit blocks
  static  std::vector<uint64_t> encodeBits(const std::vector<size_t>& positions);
  // Decode 64-bit blocks back into a list of bit positions
  static std::vector<size_t> decodeBits(const std::vector<uint64_t>& blocks);

  static uint64_t countBits(uint64_t x);

  static size_t countBitsForVector(std::vector<uint64_t> blocks);

  static bool isSimple(const std::vector<uint64_t>& blocks);
};

}  // namespace naja::NL
