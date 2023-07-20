// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_VRL_CONSTRUCTOR_UTILS_H_
#define __SNL_VRL_CONSTRUCTOR_UTILS_H_

#include "VerilogTypes.h"
#include <boost/dynamic_bitset.hpp>

namespace naja { namespace SNL {

class SNLVRLConstructorUtils {
  public:
    static void setBit(boost::dynamic_bitset<>& bits, size_t i);
    /*
     * \return a boost::dynamic_bitset version of the number
     * \warning 'x' value is converted to 0
     */
    static boost::dynamic_bitset<> numberToBits(const naja::verilog::BasedNumber& number);
};

}} // namespace SNL // namespace naja

#endif /* __SNL_VRL_CONSTRUCTOR_UTILS_H_ */
