// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_TRUTH_TABLE_H_
#define __SNL_TRUTH_TABLE_H_

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "BitVecDynamic.h"
#include "NLException.h"

namespace naja {
namespace NL {

class SNLTruthTable {
 public:
  SNLTruthTable() : size_(std::numeric_limits<uint32_t>::max()) {}

  // Enforce size ≤ 6 BEFORE touching BitVecDynamic
  explicit SNLTruthTable(uint32_t size, uint64_t bits) {
    if (size > 6) {
      std::ostringstream oss;
      oss << "Cannot create SNLTruthTable with bits_: " << bits
          << " and size: " << size << " (max=6)";
      throw NLException(oss.str());
    }
    size_ = size;
    // now safe: 1<<size <= 64
    bits_ = BitVecDynamic(bits, 1u << size);
  }

  SNLTruthTable(uint32_t size, const std::vector<bool>& bits) : size_(size) {
    if (size <= 6) {
      std::ostringstream oss;
      oss << "Should use mask constructor for 6 or less inputs";
      throw NLException(oss.str());
    }
    size_ = size;
    // now safe: 1<<size <= 64
    bits_ = BitVecDynamic(bits, 1u << size);
  }

  static SNLTruthTable Logic0() { return SNLTruthTable(0, 0); }
  static SNLTruthTable Logic1() { return SNLTruthTable(0, 1); }
  static SNLTruthTable Inv() { return SNLTruthTable(1, 0b01); }
  static SNLTruthTable Buf() { return SNLTruthTable(1, 0b10); }

  bool operator==(const SNLTruthTable& o) const {
    return size_ == o.size_ && bits_ == o.bits_;
  }

  bool operator<(const SNLTruthTable& o) const {
    if (size_ != o.size_)
      return size_ < o.size_;
    return bits_ < o.bits_;
  }

  std::string getString() const {
    return "<" + std::to_string(size_) + ", " + std::to_string(bits()) + ">";
  }

  bool isInitialized() const {
    return size_ != std::numeric_limits<uint32_t>::max();
  }

  using ConstantInput = std::pair<uint32_t, bool>;
  using ConstantInputs = std::vector<ConstantInput>;

  // Apply _all_ constants in one shot:
  SNLTruthTable getReducedWithConstants(ConstantInputs idxConsts) const {
    // trivial 0‐input table
    if (size_ == 0)
      return *this;

    // validate
    for (auto const& ic : idxConsts) {
      if (ic.first >= size_)
        throw NLException("Index out of range (max=6)");
    }

    // new size = old size minus #constants
    uint32_t k = static_cast<uint32_t>(idxConsts.size());
    uint32_t newSize = (size_ > k ? size_ - k : 0);
    uint32_t newN = 1u << newSize;
    uint64_t reduced = 0;

    // for each assignment 'j' of the remaining newSize bits,
    // weave in the constants to build the original index:
    for (uint32_t j = 0; j < newN; ++j) {
      uint32_t origIdx = 0;
      uint32_t remPos = 0;

      for (uint32_t bit = 0; bit < size_; ++bit) {
        bool val = false;

        // if this 'bit' is one of the constants, use that:
        auto it = std::find_if(
            idxConsts.begin(), idxConsts.end(),
            [&](ConstantInput const& c) { return c.first == bit; });
        if (it != idxConsts.end()) {
          val = it->second;
        } else {
          // otherwise pull next bit from 'j'
          val = ((j >> remPos) & 1) != 0;
          ++remPos;
        }

        origIdx |= (uint32_t(val) << bit);
      }

      // copy the matching output bit
      if (((bits() >> origIdx) & 1) != 0) {
        reduced |= (1u << j);
      }
    }

    // build & normalize result
    SNLTruthTable out(newSize, reduced);
    if (out.all0())
      return Logic0();
    if (out.all1())
      return Logic1();
    return out;
  }

  SNLTruthTable getReducedWithConstant(uint32_t idx, bool c) const {
    return getReducedWithConstants({{idx, c}});
  }

  bool hasNoInfluence(uint32_t v) const {
    auto t0 = getReducedWithConstant(v, false);
    auto t1 = getReducedWithConstant(v, true);
    return t0 == t1;
  }

  SNLTruthTable removeVariable(uint32_t v) const {
    if (v >= size_)
      throw NLException("Index out of range");
    SNLTruthTable out(size_ - 1, 0);
    uint32_t fullN = 1u << size_;

    for (uint32_t i = 0; i < fullN; ++i) {
      if (((i >> v) & 1) == 0) {
        uint32_t low = i & ((1u << v) - 1);
        uint32_t high = (i >> 1) & ~((1u << v) - 1);
        uint32_t ni = low | high;

        if (((bits() >> i) & 1) != 0)
          out.bits_ |= (1u << ni);
      }
    }
    return out;
    //LCOV_EXCL_START
  }
  //LCOV_EXCL_STOP

  bool all0() const {
    uint64_t n = 1ull << size_;
    uint64_t mask = (1ull << n) - 1ull;
    return (bits() & mask) == 0ull;
  }

  bool all1() const {
    uint64_t n = 1ull << size_;
    uint64_t mask = (1ull << n) - 1ull;
    return (bits() & mask) == mask;
  }

  uint32_t size() const { return size_; }
  uint64_t bits() const { return static_cast<uint64_t>(bits_); }

 private:
  uint32_t size_{std::numeric_limits<uint32_t>::max()};
  BitVecDynamic bits_{0, /*length=*/1};
};

}  // namespace NL
}  // namespace naja

#endif  // __SNL_TRUTH_TABLE_H_
