// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

// NLBitVecDynamic.h
// ————————————————————————————————————————————————————————————————————————————————
// A truth-table bit-vector that uses a uint64_t under the hood
// whenever size ≤ 64 bits (i.e. ≤ 6 inputs ⇒ 2^6=64), and
// otherwise falls back to a packed std::vector<bool>.
// Exposes: >>, |=, &, ==, <, and conversion to uint64_t.
// Fully RAII-safe via std::variant, no manual union management.

#ifndef NAJA_NL_BitVecDynamic_H_
#define NAJA_NL_BitVecDynamic_H_

#include <cstdint>
#include <vector>
#include <variant>
#include <algorithm>
#include "NLException.h"

namespace naja { namespace NL {

class NLBitVecDynamic {
public:
  // Default: zero-length; stores in the uint64_t variant
  NLBitVecDynamic()
    : nbits_(0), data_(uint64_t{0})
  {}

  // Construct from a 64-bit mask; length must be ≤ 64
  explicit NLBitVecDynamic(uint64_t mask, uint32_t length)
    : nbits_(length)
  {
    if (length <= 64) {
      data_ = mask;
    } else {
      throw NLException(
        "NLBitVecDynamic: cannot construct from mask when length>64");
    }
  }

  // Construct a zero-filled vector of given length
  explicit NLBitVecDynamic(const std::vector<bool>& bits, uint32_t length)
    : nbits_(length)
  {
    if (length > 64) {
      data_ = bits;
    } else {
      throw NLException(
        "When under size of 64, use mask constructor.");
    }
  }

  // Construct a zero-filled vector of given length
  explicit NLBitVecDynamic(uint32_t length)
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
  NLBitVecDynamic& operator|=(uint64_t mask) {
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

  // AND with mask, returns a fresh NLBitVecDynamic
  NLBitVecDynamic operator&(uint64_t mask) const {
    if (std::holds_alternative<uint64_t>(data_)) {
      return NLBitVecDynamic(std::get<uint64_t>(data_) & mask, nbits_);
    } else {
      NLBitVecDynamic out(nbits_);
      auto const &vec = std::get<std::vector<bool>>(data_);
      auto &ovec      = std::get<std::vector<bool>>(out.data_);
      for (uint32_t i = 0; i < std::min<uint32_t>(nbits_, 64U); ++i)
        ovec[i] = vec[i] && (((mask >> i) & 1) != 0);
      return out;
    }
  }

  // Equality
  bool operator==(NLBitVecDynamic const& o) const {
    return nbits_ == o.nbits_ && data_ == o.data_;
  }

  // Lexicographic <, MSB→LSB
  bool operator<(NLBitVecDynamic const& o) const {
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
    if (std::holds_alternative<uint64_t>(data_)) {
      m = std::get<uint64_t>(data_);
    } else {
      throw NLException(
        "NLBitVecDynamic: cannot generate uint64_t for above 64 bits");
    }
    return m;
  }

  // Number of bits
  uint32_t size() const { return nbits_; }

  bool bit(size_t i) const { 
    if (nbits_ <= 64) {
      auto mask = std::get<uint64_t>(data_);
      return ((mask >> i) & 1);
    }
    auto const &vec = std::get<std::vector<bool>>(data_);
    return vec[i];
  }

  std::vector<uint64_t> getChunks() const {
    if (nbits_ <= 64) {
      std::vector<uint64_t> result;
      result.push_back(std::get<uint64_t>(data_));
      return result;
    }
    return packBits(std::get<std::vector<bool>>(data_));
  } 


private:

  // Packs 'bits' into 64-bit chunks.
  // bits.size() can be any length; the returned vector has ceiling(bits.size()/64) words.
  std::vector<uint64_t> packBits(const std::vector<bool>& bits) const {
    size_t nbits   = bits.size();
    size_t nWords  = (nbits + 63) / 64;
    std::vector<uint64_t> out(nWords, 0ULL);

    for (size_t i = 0; i < nbits; ++i) {
      if (bits[i]) {
        size_t wordIdx = i >> 6;        // i / 64
        size_t bitPos  = i & 63;        // i % 64
        out[wordIdx] |= (uint64_t{1} << bitPos);
      }
    }

    return out;
  }

  uint32_t                               nbits_;
  std::variant<uint64_t, std::vector<bool>> data_;
};

}} // namespace naja::NL

#endif // NAJA_NL_BitVecDynamic_H_