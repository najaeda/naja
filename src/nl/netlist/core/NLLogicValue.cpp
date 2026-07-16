// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLLogicValue.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

#include "NLException.h"

namespace naja::NL {

namespace {

std::pair<bool, bool> encode(NLLogicValue value) {
  switch (value) {
    case NLLogicValue::Zero: return {false, false};
    case NLLogicValue::One:  return {true, false};
    case NLLogicValue::X:    return {true, true};
    case NLLogicValue::Z:    return {false, true};
  }
  throw NLException("NLLogicVector: invalid logic value"); // LCOV_EXCL_LINE
}

char toDigit(NLLogicValue value) {
  switch (value) {
    case NLLogicValue::Zero: return '0';
    case NLLogicValue::One:  return '1';
    case NLLogicValue::X:    return 'x';
    case NLLogicValue::Z:    return 'z';
  }
  throw NLException("NLLogicVector: invalid logic value"); // LCOV_EXCL_LINE
}

NLLogicValue fromDigit(char digit) {
  switch (digit) {
    case '0': return NLLogicValue::Zero;
    case '1': return NLLogicValue::One;
    case 'x':
    case 'X': return NLLogicValue::X;
    case 'z':
    case 'Z': return NLLogicValue::Z;
    default: throw NLException("NLLogicVector: invalid binary digit");
  }
}

}  // namespace

NLLogicVector::NLLogicVector(size_t width, NLLogicValue value):
  width_(width),
  aval_(getWordCount(width)),
  bval_(getWordCount(width)) {
  if (width == 0) {
    throw NLException("NLLogicVector: width must be greater than zero");
  }
  auto [aval, bval] = encode(value);
  std::fill(aval_.begin(), aval_.end(), aval ? std::numeric_limits<uint64_t>::max() : 0);
  std::fill(bval_.begin(), bval_.end(), bval ? std::numeric_limits<uint64_t>::max() : 0);
  maskUnusedBits();
}

void NLLogicVector::checkBitIndex(size_t lsbIndex) const {
  if (lsbIndex >= width_) {
    throw NLException("NLLogicVector: bit index out of range");
  }
}

NLLogicValue NLLogicVector::getBit(size_t lsbIndex) const {
  checkBitIndex(lsbIndex);
  const auto word = lsbIndex / 64;
  const auto mask = uint64_t(1) << (lsbIndex % 64);
  const bool aval = (aval_[word] & mask) != 0;
  const bool bval = (bval_[word] & mask) != 0;
  if (!bval) {
    return aval ? NLLogicValue::One : NLLogicValue::Zero;
  }
  return aval ? NLLogicValue::X : NLLogicValue::Z;
}

void NLLogicVector::setBit(size_t lsbIndex, NLLogicValue value) {
  checkBitIndex(lsbIndex);
  const auto word = lsbIndex / 64;
  const auto mask = uint64_t(1) << (lsbIndex % 64);
  auto [aval, bval] = encode(value);
  if (aval) {
    aval_[word] |= mask;
  } else {
    aval_[word] &= ~mask;
  }
  if (bval) {
    bval_[word] |= mask;
  } else {
    bval_[word] &= ~mask;
  }
}

bool NLLogicVector::isTwoState() const {
  return std::all_of(bval_.begin(), bval_.end(), [](uint64_t word) { return word == 0; });
}

bool NLLogicVector::containsX() const {
  for (size_t i = 0; i < aval_.size(); ++i) {
    if ((aval_[i] & bval_[i]) != 0) {
      return true;
    }
  }
  return false;
}

bool NLLogicVector::containsZ() const {
  for (size_t i = 0; i < aval_.size(); ++i) {
    if ((~aval_[i] & bval_[i]) != 0) {
      return true;
    }
  }
  return false;
}

bool NLLogicVector::isAll(NLLogicValue value) const {
  if (empty()) {
    return false;
  }
  for (size_t i = 0; i < width_; ++i) {
    if (getBit(i) != value) {
      return false;
    }
  }
  return true;
}

NLLogicVector NLLogicVector::filled(size_t width, NLLogicValue value) {
  return NLLogicVector(width, value);
}

NLLogicVector NLLogicVector::fromPackedWords(
    size_t width,
    std::vector<uint64_t> aval,
    std::vector<uint64_t> bval) {
  if (width == 0) {
    throw NLException("NLLogicVector: width must be greater than zero");
  }
  const auto expectedWords = getWordCount(width);
  if (aval.size() != expectedWords || bval.size() != expectedWords) {
    throw NLException("NLLogicVector: packed word count does not match width");
  }
  NLLogicVector result(width);
  result.aval_ = std::move(aval);
  result.bval_ = std::move(bval);
  const auto originalAval = result.aval_;
  const auto originalBval = result.bval_;
  result.maskUnusedBits();
  if (result.aval_ != originalAval || result.bval_ != originalBval) {
    throw NLException("NLLogicVector: unused packed bits must be zero");
  }
  return result;
}

NLLogicVector NLLogicVector::fromVerilogBinary(std::string_view literal) {
  const auto quote = literal.find('\'');
  if (quote == std::string_view::npos || quote == 0 || quote + 2 > literal.size()) {
    throw NLException("NLLogicVector: expected a width-qualified binary literal");
  }

  size_t width = 0;
  for (size_t i = 0; i < quote; ++i) {
    const auto c = static_cast<unsigned char>(literal[i]);
    if (!std::isdigit(c)) {
      throw NLException("NLLogicVector: invalid binary literal width");
    }
    const auto digit = static_cast<size_t>(literal[i] - '0');
    if (width > (std::numeric_limits<size_t>::max() - digit) / 10) {
      throw NLException("NLLogicVector: binary literal width overflow");
    }
    width = width * 10 + digit;
  }
  if (width == 0) {
    throw NLException("NLLogicVector: width must be greater than zero");
  }

  const char base = literal[quote + 1];
  if (base != 'b' && base != 'B') {
    throw NLException("NLLogicVector: only binary literals are supported");
  }

  std::string digits;
  digits.reserve(literal.size() - quote - 2);
  for (size_t i = quote + 2; i < literal.size(); ++i) {
    if (literal[i] == '_') {
      if (digits.empty() || i + 1 == literal.size() || literal[i + 1] == '_') {
        throw NLException("NLLogicVector: underscore must separate binary digits");
      }
      continue;
    }
    digits.push_back(literal[i]);
  }
  if (digits.size() != width) {
    throw NLException("NLLogicVector: binary literal width does not match its digits");
  }

  auto result = filled(width, NLLogicValue::Zero);
  for (size_t i = 0; i < width; ++i) {
    result.setBit(i, fromDigit(digits[width - i - 1]));
  }
  return result;
}

std::string NLLogicVector::toBinaryDigits() const {
  if (empty()) {
    return {};
  }
  std::string result;
  result.reserve(width_);
  for (size_t i = width_; i > 0; --i) {
    result.push_back(toDigit(getBit(i - 1)));
  }
  return result;
}

std::string NLLogicVector::toVerilogBinary() const {
  if (empty()) {
    throw NLException("NLLogicVector: cannot format an empty value");
  }
  return std::to_string(width_) + "'b" + toBinaryDigits();
}

void NLLogicVector::maskUnusedBits() {
  const auto used = width_ % 64;
  if (used == 0 || aval_.empty()) {
    return;
  }
  const auto mask = (uint64_t(1) << used) - 1;
  aval_.back() &= mask;
  bval_.back() &= mask;
}

}  // namespace naja::NL
