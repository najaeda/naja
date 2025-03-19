// Copyright 2023 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLConstructorUtils.h"

#include <sstream>
#include "SNLVRLConstructorException.h"

namespace naja { namespace NL {

void SNLVRLConstructorUtils::setBit(boost::dynamic_bitset<>& bits, size_t i) {
  if (i<bits.size()) {
    bits.set(i);
  }
}

boost::dynamic_bitset<> SNLVRLConstructorUtils::numberToBits(const naja::verilog::BasedNumber& number) {
  //Transform constant number to bitset 
  // Verilog MSB:LSB
  // dynamic_bitset: bit 0 is LSB
  switch (number.base_) {
    case naja::verilog::BasedNumber::DECIMAL: {
      unsigned long value = std::stoul(number.digits_);
      auto bits = boost::dynamic_bitset<>(number.size_, value);
      return bits;
    }
    case naja::verilog::BasedNumber::BINARY: {
      boost::dynamic_bitset<> bits(number.size_, 0ul);
      for (int i = number.digits_.size()-1; i>=0; i--) {
        const char& c = number.digits_[i];
        switch (toupper(c)) {
          case '0': case 'X': break;
          case '1': setBit(bits, i); break;
          default: {
            std::stringstream stream;
            stream << "In SNLVRLConstructorUtils::numberToBits, unrecognized binary character: " << c;
            throw naja::NL::SNLVRLConstructorException(stream.str());
          }
        }
      }
      return bits;
    }
    case naja::verilog::BasedNumber::HEX: {
      boost::dynamic_bitset<> bits(number.size_, 0ul);
      //visit from LSB to MSB, collect slices of 4 bits
      size_t i = 0; 
      for (int j = number.digits_.size()-1; j>=0; j--) {
        const char& c = number.digits_[j];
        switch (toupper(c)) {
          case '0': case 'X': break;
          case '1': setBit(bits, i); break;
          case '2': setBit(bits, i+1); break;
          case '3': setBit(bits, i+1); setBit(bits,i); break;
          case '4': setBit(bits, i+2); break;
          case '5': setBit(bits, i+2); setBit(bits, i); break;
          case '6': setBit(bits, i+2); setBit(bits, i+1); break;
          case '7': setBit(bits, i+2); setBit(bits, i+1); setBit(bits, i); break;
          case '8': setBit(bits, i+3); break;
          case '9': setBit(bits, i+3); setBit(bits, i); break;
          case 'A': setBit(bits, i+3); setBit(bits, i+1); break;
          case 'B': setBit(bits, i+3); setBit(bits, i+1); setBit(bits, i); break;
          case 'C': setBit(bits, i+3); setBit(bits, i+2); break;
          case 'D': setBit(bits, i+3); setBit(bits, i+2); setBit(bits, i); break;
          case 'E': setBit(bits, i+3); setBit(bits, i+2); setBit(bits, i+1); break;
          case 'F': setBit(bits, i+3); setBit(bits, i+2); setBit(bits, i+1); setBit(bits, i); break;
          default: {
            std::stringstream stream;
            stream << "In SNLVRLConstructorUtils::numberToBits, unrecognized Hexadecimal character: " << c;
            throw naja::NL::SNLVRLConstructorException(stream.str());
          }
        }
        i+=4;
        if (i>number.size_) {
          break;
        }
      }
      return bits;
    }
    default: {
      std::stringstream stream;
      stream << "In SNLVRLConstructorUtils::numberToBits, unsupported format: "
        << naja::verilog::BasedNumber::getBaseString(number.base_);
      throw naja::NL::SNLVRLConstructorException(stream.str());
    }
  }
  return boost::dynamic_bitset<>();
}

}} // namespace SNL // namespace naja
