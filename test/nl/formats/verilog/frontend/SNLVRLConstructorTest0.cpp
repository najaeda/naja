// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLVRLConstructor.h"
#include "SNLVRLConstructorException.h"

using namespace naja::NL;

TEST(SNLVRLConstructorTest0, test1) {
  EXPECT_EQ(SNLTerm::Direction::Input,
    SNLVRLConstructor::VRLDirectionToSNLDirection(naja::verilog::Port::Direction::Input));
  EXPECT_EQ(SNLTerm::Direction::Output,
    SNLVRLConstructor::VRLDirectionToSNLDirection(naja::verilog::Port::Direction::Output));
  EXPECT_EQ(SNLTerm::Direction::InOut,
    SNLVRLConstructor::VRLDirectionToSNLDirection(naja::verilog::Port::Direction::InOut));
  EXPECT_THROW(
    SNLVRLConstructor::VRLDirectionToSNLDirection(naja::verilog::Port::Direction::Unknown),
    SNLVRLConstructorException
  );
}
