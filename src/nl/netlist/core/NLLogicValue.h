// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace naja::NL {

/// \brief One bit of four-state logic.
enum class NLLogicValue : uint8_t {
  Zero,
  One,
  X,
  Z
};

/// \brief Structural origin of a constant driver.
enum class NLConstantDriverKind : uint8_t {
  Assign,
  Supply
};

/**
 * \brief Width-exact packed four-state value.
 *
 * Bits are indexed least-significant bit first. Storage uses the conventional
 * aval/bval encoding: 0=00, 1=10, X=11, Z=01.
 */
class NLLogicVector {
  public:
    NLLogicVector() = default;
    explicit NLLogicVector(size_t width, NLLogicValue value = NLLogicValue::Zero);

    size_t getWidth() const { return width_; }
    bool empty() const { return width_ == 0; }

    NLLogicValue getBit(size_t lsbIndex) const;
    void setBit(size_t lsbIndex, NLLogicValue value);

    bool isTwoState() const;
    bool isFullyKnown() const { return isTwoState(); }
    bool containsX() const;
    bool containsZ() const;
    bool isAll(NLLogicValue value) const;

    /// \brief Construct a non-empty vector filled with \p value.
    static NLLogicVector filled(size_t width, NLLogicValue value);

    /// \brief Construct a vector from packed aval/bval storage.
    static NLLogicVector fromPackedWords(
      size_t width,
      std::vector<uint64_t> aval,
      std::vector<uint64_t> bval);

    /**
     * \brief Parse a width-qualified binary Verilog literal.
     *
     * Accepted digits are 0, 1, x, X, z, and Z. Underscores between digits
     * are ignored. The number of digits must exactly match the declared width.
     */
    static NLLogicVector fromVerilogBinary(std::string_view literal);

    /// \return Canonical width-qualified binary literal, with lower-case x/z.
    std::string toVerilogBinary() const;
    /// \return Canonical MSB-to-LSB binary digits, with lower-case x/z.
    std::string toBinaryDigits() const;

    const std::vector<uint64_t>& getAvalWords() const { return aval_; }
    const std::vector<uint64_t>& getBvalWords() const { return bval_; }

    bool operator==(const NLLogicVector& other) const {
      return width_ == other.width_ && aval_ == other.aval_ && bval_ == other.bval_;
    }
    bool operator!=(const NLLogicVector& other) const { return !(*this == other); }

  private:
    static size_t getWordCount(size_t width) { return (width + 63) / 64; }
    void checkBitIndex(size_t lsbIndex) const;
    void maskUnusedBits();

    size_t                width_ {0};
    std::vector<uint64_t> aval_  {};
    std::vector<uint64_t> bval_  {};
};

struct NLConstantDriver {
  NLLogicVector value;
  NLConstantDriverKind kind {NLConstantDriverKind::Assign};

  bool operator==(const NLConstantDriver&) const = default;
};

}  // namespace naja::NL
