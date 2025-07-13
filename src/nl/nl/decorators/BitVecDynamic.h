// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
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
    : _nbits(0), _data(uint64_t{0})
  {}

  // Construct from a 64-bit mask; length must be ≤ 64
  explicit BitVecDynamic(uint64_t mask, uint32_t length)
    : _nbits(length)
  {
    if (length <= 64) {
      _data = mask;
    } else {
      throw std::out_of_range(
        "BitVecDynamic: cannot construct from mask when length>64");
    }
  }

  // Construct a zero-filled vector of given length
  explicit BitVecDynamic(uint32_t length)
    : _nbits(length)
  {
    if (length <= 64) {
      _data = uint64_t{0};
    } else {
      _data = std::vector<bool>(length, false);
    }
  }

  // Extract bit at pos (old “(bits_ >> pos)&1”)
  bool operator>>(size_t pos) const {
    if (pos >= _nbits) return false;
    if (std::holds_alternative<uint64_t>(_data)) {
      return (std::get<uint64_t>(_data) >> pos) & 1;
    } else {
      return std::get<std::vector<bool>>(_data)[pos];
    }
  }

  // OR a 64-bit mask in (old “bits_|=(1ULL<<i)”)
  BitVecDynamic& operator|=(uint64_t mask) {
    if (std::holds_alternative<uint64_t>(_data)) {
      _data = std::get<uint64_t>(_data) | mask;
    } else {
      auto &vec = std::get<std::vector<bool>>(_data);
      for (size_t i = 0; i < std::min<uint32_t>(_nbits, 64U); ++i)
        if ((mask >> i) & 1)
          vec[i] = true;
    }
    return *this;
  }

  // AND with mask, returns a fresh BitVecDynamic
  BitVecDynamic operator&(uint64_t mask) const {
    if (std::holds_alternative<uint64_t>(_data)) {
      return BitVecDynamic(std::get<uint64_t>(_data) & mask, _nbits);
    } else {
      BitVecDynamic out(_nbits);
      auto const &vec = std::get<std::vector<bool>>(_data);
      auto &ovec      = std::get<std::vector<bool>>(out._data);
      for (uint32_t i = 0; i < std::min<uint32_t>(_nbits, 64U); ++i)
        ovec[i] = vec[i] && (((mask >> i) & 1) != 0);
      return out;
    }
  }

  // Equality
  bool operator==(BitVecDynamic const& o) const {
    if (_nbits != o._nbits) return false;
    return _data == o._data;
  }

  // Lexicographic <, MSB→LSB
  bool operator<(BitVecDynamic const& o) const {
    // If both fit in uint64_t, do a direct integer compare
    if (std::holds_alternative<uint64_t>(_data) &&
        std::holds_alternative<uint64_t>(o._data)) {
      return std::get<uint64_t>(_data) < std::get<uint64_t>(o._data);
    }

    // Otherwise compare bit-by-bit, MSB first
    uint32_t maxb = std::max(_nbits, o._nbits);
    for (uint32_t off = 0; off < maxb; ++off) {
      size_t pos = maxb - 1 - off;
      bool b1 = (pos < _nbits) ? ((*this) >> pos) : false;
      bool b2 = (pos < o._nbits) ? (o >> pos)        : false;
      if (b1 != b2) return !b1 && b2;
    }
    return false;
  }

  // Convert back to uint64_t (old “static_cast<uint64_t>(bits_)”)
  explicit operator uint64_t() const {
    uint64_t m = 0;
    uint32_t upto = std::min<uint32_t>(_nbits, 64U);
    if (std::holds_alternative<uint64_t>(_data)) {
      m = std::get<uint64_t>(_data);
    } else {
      auto const &vec = std::get<std::vector<bool>>(_data);
      for (uint32_t i = 0; i < upto; ++i)
        if (vec[i]) m |= (uint64_t{1} << i);
    }
    return m;
  }

  // Number of bits
  uint32_t size() const { return _nbits; }

private:
  uint32_t                              _nbits;
  std::variant<uint64_t, std::vector<bool>> _data;
};

}} // namespace naja::NL

#endif // NAJA_NL_BITVECDYNAMIC_H_
