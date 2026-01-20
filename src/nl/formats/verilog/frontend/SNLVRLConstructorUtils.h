// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "VerilogTypes.h"
#include <boost/dynamic_bitset.hpp>

namespace naja::NL {

class SNLVRLConstructorUtils {
  public:
    static void setBit(boost::dynamic_bitset<>& bits, size_t i);
    /*
     * \return a boost::dynamic_bitset version of the number
     * \warning 'x' value is converted to 0
     */
    static boost::dynamic_bitset<> numberToBits(const naja::verilog::BasedNumber& number);
};

}  // namespace naja::NL