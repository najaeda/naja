/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
