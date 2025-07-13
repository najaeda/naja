// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

// BitVecDynamic.h
// ————————————————————————————————————————————————————————————————————————————————
// A truth-table bit-vector that uses a uint64_t under the hood
// whenever size ≤ 64 bits (i.e. ≤ 6 inputs ⇒ 2^6=64), and
// otherwise falls back to a packed std::vector<bool>.
// Exposes: >>, |=, &, ==, <, and conversion to uint64_t.
// Fully RAII-safe via std::variant, no manual union management.

#ifndef NAJA_NL_BITVECDYNAMIC_H_
#define NAJA_NL_BITVECDYNAMIC_H_

#include <cstdint>
#include <vector>
#include <variant>
#include <algorithm>
#include <stdexcept>

namespace naja { namespace NL {

class BitVecDynamic {
public:
  // Default: zero-length; stores in the uint64_t variant
  BitVecDynamic()
    : nbits_(0), data_(uint64_t{0})
  {}

  // Construct from a 64-bit mask; length must be ≤ 64
  explicit BitVecDynamic(uint64_t mask, uint32_t length)
    : nbits_(length)
  {
    if (length <= 64) {
      data_ = mask;
    } else {
      throw std::out_of_range(
        "BitVecDynamic: cannot construct from mask when length>64");
    }
  }

  // Construct a zero-filled vector of given length
  explicit BitVecDynamic(uint32_t length)
    : nbits_(length)
  {
    if (length <= 64) {
      data_ = uint64_t{0};
    } else {
      data_ = std::vector<bool>(length, false);
    }
  }

  // Extract bit at pos (old “(bits_ >> pos)&1”)
  bool operator>>(size_t pos) const {
    if (pos >= nbits_) return false;
    if (std::holds_alternative<uint64_t>(data_)) {
      return (std::get<uint64_t>(data_) >> pos) & 1;
    } else {
      return std::get<std::vector<bool>>(data_)[pos];
    }
  }

  // OR a 64-bit mask in (old “bits_|=(1ULL<<i)”)
  BitVecDynamic& operator|=(uint64_t mask) {
    if (std::holds_alternative<uint64_t>(data_)) {
      data_ = std::get<uint64_t>(data_) | mask;
    } else {
      auto &vec = std::get<std::vector<bool>>(data_);
      for (size_t i = 0; i < std::min<uint32_t>(nbits_, 64U); ++i)
        if ((mask >> i) & 1)
          vec[i] = true;
    }
    return *this;
  }

  // AND with mask, returns a fresh BitVecDynamic
  BitVecDynamic operator&(uint64_t mask) const {
    if (std::holds_alternative<uint64_t>(data_)) {
      return BitVecDynamic(std::get<uint64_t>(data_) & mask, nbits_);
    } else {
      BitVecDynamic out(nbits_);
      auto const &vec = std::get<std::vector<bool>>(data_);
      auto &ovec      = std::get<std::vector<bool>>(out.data_);
      for (uint32_t i = 0; i < std::min<uint32_t>(nbits_, 64U); ++i)
        ovec[i] = vec[i] && (((mask >> i) & 1) != 0);
      return out;
    }
  }

  // Equality
  bool operator==(BitVecDynamic const& o) const {
    return nbits_ == o.nbits_ && data_ == o.data_;
  }

  // Lexicographic <, MSB→LSB
  bool operator<(BitVecDynamic const& o) const {
    if (std::holds_alternative<uint64_t>(data_) &&
        std::holds_alternative<uint64_t>(o.data_)) {
      return std::get<uint64_t>(data_) < std::get<uint64_t>(o.data_);
    }
    uint32_t maxb = std::max(nbits_, o.nbits_);
    for (uint32_t off = 0; off < maxb; ++off) {
      size_t pos = maxb - 1 - off;
      bool b1 = (pos < nbits_) ? ((*this) >> pos) : false;
      bool b2 = (pos < o.nbits_) ? (o >> pos)        : false;
      if (b1 != b2) return !b1 && b2;
    }
    return false;
  }

  // Convert back to uint64_t (old “static_cast<uint64_t>(bits_)”)
  explicit operator uint64_t() const {
    uint64_t m    = 0;
    uint32_t upto = std::min<uint32_t>(nbits_, 64U);
    if (std::holds_alternative<uint64_t>(data_)) {
      m = std::get<uint64_t>(data_);
    } else {
      auto const &vec = std::get<std::vector<bool>>(data_);
      for (uint32_t i = 0; i < upto; ++i)
        if (vec[i]) m |= (uint64_t{1} << i);
    }
    return m;
  }

  // Number of bits
  uint32_t size() const { return nbits_; }

private:
  uint32_t                               nbits_;
  std::variant<uint64_t, std::vector<bool>> data_;
};

}} // namespace naja::NL

#endif // NAJA_NL_BITVECDYNAMIC_H_