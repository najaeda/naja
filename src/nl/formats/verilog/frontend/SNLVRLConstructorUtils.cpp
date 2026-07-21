// Copyright 2023 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLConstructorUtils.h"

#include <cctype>
#include <sstream>
#include "SNLVRLConstructorException.h"

namespace naja::NL {

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

std::vector<SNLNet::Type> SNLVRLConstructorUtils::numberToNetTypes(
  const naja::verilog::BasedNumber& number) {
  std::vector<SNLNet::Type> bits(number.size_, SNLNet::Type::Assign0);
  const auto setHexDigit = [&](size_t offset, char digit) {
    const auto setKnownDigit = [&](unsigned value) {
      for (size_t bit = 0; bit < 4 && offset + bit < bits.size(); ++bit) {
        bits[offset + bit] = (value & (1U << bit))
          ? SNLNet::Type::Assign1
          : SNLNet::Type::Assign0;
      }
    };
    const auto upper = static_cast<char>(std::toupper(static_cast<unsigned char>(digit)));
    if (upper >= '0' && upper <= '9') {
      setKnownDigit(static_cast<unsigned>(upper - '0'));
    } else if (upper >= 'A' && upper <= 'F') {
      setKnownDigit(static_cast<unsigned>(upper - 'A' + 10));
    } else if (upper == 'X' || upper == 'Z') {
      const auto type = upper == 'X' ? SNLNet::Type::AssignX : SNLNet::Type::AssignZ;
      for (size_t bit = 0; bit < 4 && offset + bit < bits.size(); ++bit) {
        bits[offset + bit] = type;
      }
    } else {
      std::stringstream stream;
      stream << "In SNLVRLConstructorUtils::numberToNetTypes, unrecognized hexadecimal character: "
             << digit;
      throw SNLVRLConstructorException(stream.str());
    }
  };

  switch (number.base_) {
    case naja::verilog::BasedNumber::DECIMAL: {
      const auto binaryBits = numberToBits(number);
      for (size_t bit = 0; bit < binaryBits.size(); ++bit) {
        bits[bit] = binaryBits[bit] ? SNLNet::Type::Assign1 : SNLNet::Type::Assign0;
      }
      return bits;
    }
    case naja::verilog::BasedNumber::BINARY: {
      size_t index = 0;
      for (auto it = number.digits_.rbegin();
           it != number.digits_.rend() && index < bits.size(); ++it, ++index) {
        const auto upper = static_cast<char>(
          std::toupper(static_cast<unsigned char>(*it)));
        switch (upper) {
          case '0': bits[index] = SNLNet::Type::Assign0; break;
          case '1': bits[index] = SNLNet::Type::Assign1; break;
          case 'X': bits[index] = SNLNet::Type::AssignX; break;
          case 'Z': bits[index] = SNLNet::Type::AssignZ; break;
          default: {
            std::stringstream stream;
            stream << "In SNLVRLConstructorUtils::numberToNetTypes, unrecognized binary character: "
                   << *it;
            throw SNLVRLConstructorException(stream.str());
          }
        }
      }
      return bits;
    }
    case naja::verilog::BasedNumber::HEX: {
      size_t offset = 0;
      for (auto it = number.digits_.rbegin(); it != number.digits_.rend() && offset < bits.size();
           ++it, offset += 4) {
        setHexDigit(offset, *it);
      }
      return bits;
    }
    default: {
      std::stringstream stream;
      stream << "In SNLVRLConstructorUtils::numberToNetTypes, unsupported format: "
             << naja::verilog::BasedNumber::getBaseString(number.base_);
      throw SNLVRLConstructorException(stream.str());
    }
  }
}

} // namespace naja::NL
