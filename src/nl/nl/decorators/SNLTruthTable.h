// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_TRUTH_TABLE_H_
#define __SNL_TRUTH_TABLE_H_

#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

#include "NLException.h"

namespace naja { namespace NL {

class SNLTruthTable {
  public:
    SNLTruthTable(): size_(std::numeric_limits<uint32_t>::max()) {}
    explicit SNLTruthTable(uint32_t size, uint64_t bits): size_(size), bits_(bits) {
      if (size > 6) {
        std::ostringstream oss;
        oss << "Cannot create SNLTruthTable with bits_: " << bits;
        oss << " and size: " << size;
        oss << " (max=6)";
        throw NLException(oss.str());
      }
    }

    static SNLTruthTable Logic0() {
      return SNLTruthTable(0, 0);
    }

    static SNLTruthTable Logic1() {
      return SNLTruthTable(0, 1);
    }

    static SNLTruthTable Inv() {
      return SNLTruthTable(1, 0b01);
    }

    static SNLTruthTable Buf() {
      return SNLTruthTable(1, 0b10);
    }

    bool operator ==(const SNLTruthTable& other) const {
      return size_ == other.size_ and bits_ == other.bits_;	    
    }
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
      return size_ != std::numeric_limits<uint32_t>::max();
    }

    using ConstantInput = std::pair<uint32_t, bool>;
    using ConstantInputs = std::vector<ConstantInput>;
    SNLTruthTable getReducedWithConstants(ConstantInputs indexConstants) const {
      // If the truth table is empty, return itself
      if (size_ == 0) {
        return *this;
      }

      sort(indexConstants.begin(), indexConstants.end(),
        [](const ConstantInput& l, const ConstantInput& r) {
        return l.first > r.first;
      });

      // Create a copy of the current truth table
      SNLTruthTable currentTT = *this;

      // Iterate through each pair of index and constant
      for (const auto& indexConstant: indexConstants) {
        uint32_t index = indexConstant.first;
        bool constant = indexConstant.second;

        // Check if the index is out of range
        if (index > currentTT.size_ - 1) {
          throw NLException("Index out of range (max=6)");
        }

        // Calculate the number of entries in the truth table
        uint32_t n = 1U << currentTT.size_;
        uint64_t reducedBits = 0;
        uint32_t bitPos = 0;

        // Iterate over all possible input combinations
        for (uint32_t i = 0; i < n; ++i) {
          // Check if the index-th bit matches the constant
          if (((i >> index) & 1) == constant) {
            // Copy the corresponding bit from the original truth table
            reducedBits |= ((currentTT.bits_ >> i) & 1) << bitPos;
            ++bitPos;
          }
        }

        // Update the current truth table with the reduced size and bits
        currentTT = SNLTruthTable(currentTT.size_ - 1, reducedBits);

        // Check if the new truth table represents a constant 0 or 1
        if (currentTT.all0()) {
          return SNLTruthTable::Logic0();
        }
        if (currentTT.all1()) {
          return SNLTruthTable::Logic1();
        }
      }
      // Return the final reduced truth table
      return currentTT;
    }

    SNLTruthTable getReducedWithConstant(uint32_t index, bool constant) const {
      return getReducedWithConstants({{index, constant}});
    }

    // Function to check if an input has no influence on the output
    bool hasNoInfluence(uint32_t variableIndex) const {
      SNLTruthTable tableWithZero = getReducedWithConstant(variableIndex, false);
      SNLTruthTable tableWithOne = getReducedWithConstant(variableIndex, true);
      return tableWithZero == tableWithOne;
    }    

    // Function to remove a variable from the truth table
    SNLTruthTable removeVariable(uint32_t variableIndex) const {
      if (variableIndex > size_-1) {
        throw NLException("Index out of range");
      }
      SNLTruthTable reducedTruthTable(size_-1, 0);
      for (uint32_t i = 0; i < (1U << size_); ++i) {
        if (((i >> variableIndex) & 1) == 0) {
            uint32_t newIdx = ((i & ((1U << variableIndex) - 1U)) | ((i >> 1U) & (~((1U << variableIndex) - 1U))));
            if (((bits_ >> i) & 1) == 1) {
              reducedTruthTable.bits_ |= (1U << newIdx);
            }
        }
      }
      return reducedTruthTable;
    }

    bool all0() const {
      uint64_t n = 1ULL << size_;
      uint64_t mask = (1ULL << n) - 1ULL;
      uint64_t result = bits_ & mask;
      return result == 0;
    }

    bool all1() const {
      uint64_t n = 1ULL << size_;
      uint64_t mask = (1ULL << n) - 1ULL;
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
    uint32_t  size_ {std::numeric_limits<uint32_t>::max()};
    uint64_t  bits_ {0};
};

}} // namespace NL // namespace naja

#endif /* __SNL_TRUTH_TABLE_H_ */
