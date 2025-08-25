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

#include "NLBitVecDynamic.h"
#include "NLException.h"
#include "NLBitDependencies.h"

namespace naja {
namespace NL {

class SNLTruthTable {
 public:
  SNLTruthTable() : size_(std::numeric_limits<uint32_t>::max()) {}

  // user‐provided copy‐ctor
  SNLTruthTable(const SNLTruthTable& o)
    : size_(o.size_),
      bits_(o.bits_),
      dependencies_(o.dependencies_)
  {}

  // explicit copy‐assignment to match
  SNLTruthTable& operator=(const SNLTruthTable& o)
  {
    if (this != &o) {
      size_ = o.size_;
      bits_ = o.bits_;
      dependencies_ = o.dependencies_;
    }
    return *this;
  }

  // Enforce size ≤ 6 BEFORE touching BitVecDynamic
  explicit SNLTruthTable(uint32_t size, uint64_t bits, const std::vector<uint64_t>& dependencies = std::vector<uint64_t>()) : dependencies_(dependencies) {
    if (size > 6) {
      std::ostringstream oss;
      oss << "Cannot create SNLTruthTable with bits_: " << bits
          << " and size: " << size << " (max=6)";
      throw NLException(oss.str());
    }
    size_ = size;
    // now safe: 1<<size <= 64
    bits_ = NLBitVecDynamic(bits, 1u << size);
    if (dependencies_.empty()) {
      dependencies_ = std::vector<uint64_t>(size / 64 + ((size % 64) > 0 ? 1 : 0), 0);
    }
    if (dependencies_.size() != size / 64 + ((size % 64) > 0 ? 1 : 0)) {
      std::ostringstream oss;
      oss << "Dependencies size mismatch: expected "
          << (size / 64 + ((size % 64) > 0 ? 1 : 0))
          << ", got " << dependencies_.size();
      throw NLException(oss.str());
    }
  }

  SNLTruthTable(uint32_t size, const std::vector<bool>& bits, const std::vector<uint64_t>& dependencies = std::vector<uint64_t>()) : size_(size), dependencies_(dependencies) {
    if (size <= 6) {
      std::ostringstream oss;
      oss << "Should use mask constructor for 6 or less inputs";
      throw NLException(oss.str());
    }
    size_ = size;
    // now safe: 1<<size <= 64
    bits_ = NLBitVecDynamic(bits, 1u << size);
    if (dependencies_.empty()) {
      dependencies_ = std::vector<uint64_t>(size / 64 + ((size % 64) > 0 ? 1 : 0), 0);
    }
    if (dependencies_.size() != size / 64 + ((size % 64) > 0 ? 1 : 0)) {
      std::ostringstream oss;
      oss << "Dependencies size mismatch: expected "
          << (size / 64 + ((size % 64) > 0 ? 1 : 0))
          << ", got " << dependencies_.size();
      throw NLException(oss.str());
    }
  }

  static SNLTruthTable Logic0() { return SNLTruthTable(0, 0); }
  static SNLTruthTable Logic1() { return SNLTruthTable(0, 1); }
  static SNLTruthTable Inv() { return SNLTruthTable(1, 0b01); }
  static SNLTruthTable Buf() { return SNLTruthTable(1, 0b10); }

  bool operator==(const SNLTruthTable& o) const {
    return size_ == o.size_ && bits_ == o.bits_ && dependencies_ == o.dependencies_;
  }

  bool operator<(const SNLTruthTable& o) const {
    if (size_ != o.size_)
      return size_ < o.size_;
    if (bits_ != o.bits_)
      return bits_ < o.bits_;
    return dependencies_ < o.dependencies_;
  }

  std::string getString() const {
    std::string result = "";
    result =  "<" + std::to_string(bits_.size()) + ", |"; 
    for (size_t i = 0; i < bits_.size(); i++) {
      result += std::to_string(bits_.bit(i));
    }
      
    result += "|>";
    return result;
  }

  bool isInitialized() const {
    return size_ != std::numeric_limits<uint32_t>::max();
  }

  using ConstantInput = std::pair<uint32_t, bool>;
  using ConstantInputs = std::vector<ConstantInput>;

  // Apply _all_ constants in one shot:
  SNLTruthTable getReducedWithConstants(ConstantInputs idxConsts) const {
    // trivial 0‐input table
    if (size_ == 0) {
      return *this;
    }

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
    std::vector<bool> reducedVect(newN, false);

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
      // always pull the bit via bit()
      bool inputBit = bits().bit(origIdx);

      if (newSize <= 6) {
        if (inputBit) {
          reduced |= (uint64_t{1} << j);
        }
      } else {
        reducedVect[j] = inputBit;
      }
    }

    // build & normalize result
    SNLTruthTable out;
    if (newSize > 6) {
      out = SNLTruthTable(newSize, reducedVect);
    } else {
      out = SNLTruthTable(newSize, reduced);
    }    
    if (out.all0()) {
      return Logic0();
    }
    if (out.all1()) {
      return Logic1();
    }
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
    if (size() <= 6) {
      uint64_t rows = 1ull << size_;    // # of table entries = 1<<size_
      uint64_t mask = (rows < 64
               ? ((1ull << rows) - 1ull)
               : std::numeric_limits<uint64_t>::max());
      return (bits().operator uint64_t() & mask) == 0ull;
    }
    bool result = false;
    for (size_t i = 0; i < bits().size(); i++) {
      result |= bits().bit(i);
    }
    return result == false;
  }

  bool all1() const {
    if (size() <= 6) {
      uint64_t rows = 1ull << size_;    // # of table entries = 1<<size_
      uint64_t mask = (rows < 64
               ? ((1ull << rows) - 1ull)
               : std::numeric_limits<uint64_t>::max());
      return (bits().operator uint64_t() & mask) == mask;
    }
    bool result = true;
    for (size_t i = 0; i < bits().size(); i++) {
      result &= bits().bit(i);
    }
    return result;
  }

  uint32_t size() const { return size_; }
  const NLBitVecDynamic& bits() const { return bits_; }

  const std::vector<uint64_t>& getDependencies() const { return dependencies_; }

  std::string toString() const {
    std::ostringstream oss;
    oss << "SNLTruthTable(size=" << size_ << ", bits=" << bits_.toString()
        << ", dependencies=[";
    for (size_t i = 0; i < dependencies_.size(); ++i) {
      oss << dependencies_[i];
      if (i < dependencies_.size() - 1) {
        oss << ", ";
      }
    }
    oss << "])";
    return oss.str();
  }

 private:
  uint32_t size_{std::numeric_limits<uint32_t>::max()};
  NLBitVecDynamic bits_{0, /*length=*/1};
  std::vector<uint64_t> dependencies_{};
};

}  // namespace NL
}  // namespace naja

#endif  // __SNL_TRUTH_TABLE_H_
