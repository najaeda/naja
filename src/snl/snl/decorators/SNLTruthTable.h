// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_TRUTH_TABLE_H_
#define __SNL_TRUTH_TABLE_H_

#include <cstdint>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

#include "SNLException.h"

namespace naja { namespace SNL {

class SNLTruthTable {
  public:
    SNLTruthTable(): size_(0xFF) {}
    explicit SNLTruthTable(uint8_t size, uint64_t bits): size_(size), bits_(bits) {
      if (size > 6) {
        throw SNLException("Size out of range (max=6)");
      }
    }
    bool operator ==(const SNLTruthTable& other) const = default;
    bool operator <(const SNLTruthTable& other) const {
      if (size_ < other.size_) {
        return true;
      }
      if (size_ > other.size_) {
        return false;
      }
      return bits_ < other.bits_;
    }

    std::string getString() const {
      return "<" + std::to_string(size_) + ", " + std::to_string(bits_) + ">";
    }

    bool isInitialized() const {
      return size_ != 0xFF;
    }

    SNLTruthTable getReducedWithConstant(uint8_t index, bool constant) const {
      if (index > 6) {
        throw SNLException("Index out of range (max=6)");
      }
      if (size_ == 0) {
        return *this;
      }
      uint8_t n = (uint8_t)(1 << size_);
      uint64_t reducedBits = 0;
      uint8_t bitPos = 0;
      for (uint8_t i = 0; i < n; ++i) {
        if (((i >> index) & 1) == constant) {
            reducedBits |= ((bits_ >> i) & 1) << bitPos;
            ++bitPos;
        }
      }
      SNLTruthTable newTT(size_-1, reducedBits);
      if (newTT.is0()) {
        return SNLTruthTable(0, 0ULL);
      }
      if (newTT.is1()) {
        return SNLTruthTable(0, 1ULL);
      }
      return newTT;
    }

    // Function to check if an input has no influence on the output
    bool hasNoInfluence(uint8_t variableIndex) const {
      SNLTruthTable tableWithZero = getReducedWithConstant(variableIndex, false);
      SNLTruthTable tableWithOne = getReducedWithConstant(variableIndex, true);
      return tableWithZero == tableWithOne;
    }    

    // Function to remove a variable from the truth table
    SNLTruthTable removeVariable(uint8_t variableIndex) const {
      if (variableIndex > size_) {
        throw SNLException("Index out of range");
      }
      SNLTruthTable reducedTruthTable(size_-1, 0);
      for (uint8_t i = 0; i < (1 << size_); ++i) {
        if (((i >> variableIndex) & 1) == 0) {
            int newIdx = ((i & ((1 << variableIndex) - 1)) | ((i >> 1) & (~((1 << variableIndex) - 1))));
            if (((bits_ >> i) & 1) == 1) {
              reducedTruthTable.bits_ |= (1 << newIdx);
            }
        }
      }
      return reducedTruthTable;
    }

    bool is0() const {
      uint64_t n = 1 << size_;
      uint64_t mask = (1 << n) - 1ULL;
      uint64_t result = bits_ & mask;
      return result == 0;
    }

    bool is1() const {
      uint64_t n = 1 << size_;
      uint64_t mask = (1 << n) - 1ULL;
      uint64_t result = bits_ & mask;
      return result == mask;
    }

    uint32_t size() const {
      return size_;
    }
    
    uint64_t bits() const {
      return bits_;
    }
  private:
    uint8_t   size_ {0};
    uint64_t  bits_ {0};
};

}} // namespace SNL // namespace naja

#endif /* __SNL_TRUTH_TABLE_H_ */