// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_TRUTH_TABLE_H_
#define __SNL_TRUTH_TABLE_H_

#include <cstdint>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

#include "SNLException.h"

namespace naja { namespace SNL {

class SNLTruthTable {
  public:
    SNLTruthTable(): size_(0xFF) {}
    explicit SNLTruthTable(uint32_t size, uint64_t bits): size_(size), bits_(bits) {
      if (size > 6) {
        throw SNLException("Size out of range (max=6)");
      }
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
      return size_ != 0xFF;
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
          throw SNLException("Index out of range (max=6)");
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
        if (currentTT.is0()) {
          return SNLTruthTable(0, 0ULL);
        }
        if (currentTT.is1()) {
          return SNLTruthTable(0, 1ULL);
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
        throw SNLException("Index out of range");
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

    bool is0() const {
      uint64_t n = 1ULL << size_;
      uint64_t mask = (1ULL << n) - 1ULL;
      uint64_t result = bits_ & mask;
      return result == 0;
    }

    bool is1() const {
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
    uint32_t  size_ {0};
    uint64_t  bits_ {0};
};

}} // namespace SNL // namespace naja

#endif /* __SNL_TRUTH_TABLE_H_ */
