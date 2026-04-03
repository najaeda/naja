// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
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

namespace naja::NL {

class SNLTruthTable {
 public:
  // Enum for generic type for N input truth table { NONE, OR, NOR, AND, NAND, XOR, XNOR}
  enum class GenericType {
    NONE,
    OR,
    NOR,
    AND,
    NAND,
    XOR,
    XNOR
  };
  
  SNLTruthTable() : size_(0) {}

  static std::vector<uint64_t> fullDependencies(size_t size) {
    if (size == 0) {
      return {};
    }
    std::vector<size_t> deps(size);
    for (size_t i = 0; i < size; ++i) {
      deps[i] = i;
    }
    return NLBitDependencies::encodeBits(deps);
  }

  SNLTruthTable(size_t size,
                GenericType genericType,
                const std::vector<uint64_t>& dependencies)
      : size_(size), dependencies_(dependencies), genericType_(genericType) {
    if (size > 0 &&
        NLBitDependencies::countBitsForVector(dependencies_) > size) {
      throw NLException("Dependencies count cannot exceed truth table size");
    }
  }

  // user‐provided copy‐ctor
  SNLTruthTable(const SNLTruthTable& o)
    : size_(o.size_),
      bits_(o.bits_),
      dependencies_(o.dependencies_),
      genericType_(o.genericType_)
  {}

  // explicit copy‐assignment to match
  SNLTruthTable& operator=(const SNLTruthTable& o)
  {
    if (this != &o) {
      size_ = o.size_;
      bits_ = o.bits_;
      dependencies_ = o.dependencies_;
      genericType_ = o.genericType_;
    }
    return *this;
  }

  bool isNull() const {
    return size_ == std::numeric_limits<uint32_t>::max();
  }

  // Enforce size ≤ 6 BEFORE touching BitVecDynamic
  explicit SNLTruthTable(uint32_t size, uint64_t bits,
                         const std::vector<uint64_t>& dependencies)
      : dependencies_(dependencies) {
    if (size > 6) {
      std::ostringstream oss;
      oss << "Cannot create SNLTruthTable with bits_: " << bits
          << " and size: " << size << " (max=6)";
      throw NLException(oss.str());
    }
    size_ = size;
    // now safe: 1<<size <= 64
    bits_ = NLBitVecDynamic(bits, 1u << size);
    if (size > 0 &&
        NLBitDependencies::countBitsForVector(dependencies_) > size) {
      throw NLException("Dependencies count cannot exceed truth table size");
    }
  }

  SNLTruthTable(uint32_t size, const std::vector<bool>& bits,
                const std::vector<uint64_t>& dependencies)
      : size_(size), dependencies_(dependencies) {
    if (size <= 6) {
      std::ostringstream oss;
      oss << "Should use mask constructor for 6 or less inputs";
      throw NLException(oss.str());
    }
    size_ = size;
    // now safe: 1<<size <= 64
    bits_ = NLBitVecDynamic(bits, 1u << size);
    if (size > 0 &&
        NLBitDependencies::countBitsForVector(dependencies_) > size) {
      throw NLException("Dependencies count cannot exceed truth table size");
    }
  }

  static SNLTruthTable Logic0() { return SNLTruthTable(0, 0, {}); }
  static SNLTruthTable Logic1() { return SNLTruthTable(0, 1, {}); }
  static SNLTruthTable Inv() { return SNLTruthTable(1, 0b01, fullDependencies(1)); }
  static SNLTruthTable Buf() { return SNLTruthTable(1, 0b10, fullDependencies(1)); }

  bool operator==(const SNLTruthTable& o) const {
    return size_ == o.size_ && bits_ == o.bits_ && dependencies_ == o.dependencies_ &&
           genericType_ == o.genericType_;
  }

  bool operator<(const SNLTruthTable& o) const {
    if (size_ != o.size_)
      return size_ < o.size_;
    if (genericType_ != o.genericType_)
      return genericType_ < o.genericType_;
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
    if (size_ == 0) {
      return bits_.size() != 0;
    }
    if (dependencies_.empty()) {
      return false;
    }
    return isGeneric() || bits_.size() != 0;
  }

  bool isGeneric() const {
    return genericType_ != GenericType::NONE;
  }

  GenericType getGenericType() const {
    return genericType_;
  }

  using ConstantInput = std::pair<uint32_t, bool>;
  using ConstantInputs = std::vector<ConstantInput>;

  // Apply _all_ constants in one shot:
  SNLTruthTable getReducedWithConstants(ConstantInputs idxConsts) const {
    // Reduction relies on TT-local variable numbering, so sparse/non-local deps
    // must be canonicalized by the caller before reduction.
    if (!NLBitDependencies::isSimple(dependencies_)) {
      throw NLException(
          "getReducedWithConstants() does not support non full dependencies");
    }
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
    size_t trueConstants = 0;
    bool hasFalseConstant = false;
    for (auto const& ic : idxConsts) {
      if (ic.second) {
        ++trueConstants;
      } else {
        hasFalseConstant = true;
      }
    }

    if (isGeneric()) {
      const auto deps = fullDependencies(newSize);
      switch (genericType_) {
        case GenericType::AND:
          if (hasFalseConstant) {
            return Logic0();
          }
          if (newSize == 0) {
            return Logic1();
          }
          if (newSize == 1) {
            return Buf();
          }
          return SNLTruthTable(newSize, GenericType::AND, deps);
        case GenericType::NAND:
          if (hasFalseConstant) {
            return Logic1();
          }
          if (newSize == 0) {
            return Logic0();
          }
          if (newSize == 1) {
            return Inv();
          }
          return SNLTruthTable(newSize, GenericType::NAND, deps);
        case GenericType::OR:
          if (trueConstants > 0) {
            return Logic1();
          }
          if (newSize == 0) {
            return Logic0();
          }
          if (newSize == 1) {
            return Buf();
          }
          return SNLTruthTable(newSize, GenericType::OR, deps);
        case GenericType::NOR:
          if (trueConstants > 0) {
            return Logic0();
          }
          if (newSize == 0) {
            return Logic1();
          }
          if (newSize == 1) {
            return Inv();
          }
          return SNLTruthTable(newSize, GenericType::NOR, deps);
        case GenericType::XOR: {
          const bool invert = (trueConstants % 2) != 0;
          if (newSize == 0) {
            return invert ? Logic1() : Logic0();
          }
          if (newSize == 1) {
            return invert ? Inv() : Buf();
          }
          return SNLTruthTable(
              newSize,
              invert ? GenericType::XNOR : GenericType::XOR,
              deps);
        }
        case GenericType::XNOR: {
          const bool invert = (trueConstants % 2) != 0;
          if (newSize == 0) {
            return invert ? Logic0() : Logic1();
          }
          if (newSize == 1) {
            return invert ? Buf() : Inv();
          }
          return SNLTruthTable(
              newSize,
              invert ? GenericType::XOR : GenericType::XNOR,
              deps);
        }
        case GenericType::NONE:
          break;
      }
    }

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

    SNLTruthTable out;
    if (newSize > 6) {
      out = SNLTruthTable(newSize, reducedVect, fullDependencies(newSize));
    } else {
      out = SNLTruthTable(newSize, reduced, fullDependencies(newSize));
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
    SNLTruthTable out(size_ - 1, 0, fullDependencies(size_ - 1));
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
    if (bits().size() == 0) {
      return false;
    }
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
    if (bits().size() == 0) {
      return false;
    }
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
  uint32_t size_{0};
  NLBitVecDynamic bits_{0};
  std::vector<uint64_t> dependencies_{};
  GenericType genericType_{GenericType::NONE};
};

}  // namespace naja::NL
