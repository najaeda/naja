// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <bit>        // C++20 <bit> header
#include "NLBitDependencies.h"
#include <cassert>

// Encode a list of non-negative bit positions into 64-bit blocks.
// Blocks with no set bits at the high end are trimmed off.
// Helper function, as it does not have knowledge of inputs size. So can be used only for assertion and testing.
std::vector<uint64_t> encodeBits(const std::vector<size_t>& positions) {
    if (positions.empty()) {
        return {};
    }

    // Find highest bit to know how many blocks we need
    uint64_t maxPos = *std::max_element(positions.begin(), positions.end());
    size_t numBlocks = static_cast<size_t>(maxPos / 64) + 1;

    std::vector<uint64_t> result(numBlocks, 0);

    for (uint64_t pos : positions) {
        // safety check
        // (if you want to guard against extremely large pos, uncomment the following)
        // if (pos / 64 >= numBlocks) throw std::out_of_range("bit index too large");
        size_t blockIdx = pos / 64;
        unsigned bitIdx = static_cast<unsigned>(pos % 64);
        result[blockIdx] |= (1ULL << bitIdx);
    }

    // As length of result is highest position, it must be non zero
   assert(result.back() != 0);

    return result;
}

size_t naja::NL::NLBitDependencies::count_bits_for_vector(std::vector<uint64_t> blocks) {
    size_t count = 0;
    for (uint64_t block : blocks) {
        count += static_cast<size_t>(std::popcount(block));
    }
    return count;
}

// Decode 64-bit blocks back into a list of bit positions
std::vector<size_t> naja::NL::NLBitDependencies::decodeBits(const std::vector<uint64_t>& blocks) {
    std::vector<size_t> out;
    for (size_t blockIdx = 0; blockIdx < blocks.size(); ++blockIdx) {
        uint64_t x = blocks[blockIdx];
        while (x != 0) {
            // count trailing zeros to find the next set bit
            uint64_t bit = static_cast<uint64_t>(std::countr_zero(x));
            out.push_back((naja::NL::NLID::DesignObjectID) blockIdx * 64 + bit);
            // clear that bit
            x &= x - 1;
        }
    }
    assert(encodeBits(out).size() == blocks.size() ? encodeBits(out) == blocks : count_bits_for_vector(encodeBits(out)) == count_bits_for_vector(blocks)); // sanity check
    return out;
    // LCOV_EXCL_START
}
// LCOV_EXCL_STOP

uint64_t naja::NL::NLBitDependencies::count_bits(uint64_t x) {
    return static_cast<uint64_t>(std::popcount(x));
}