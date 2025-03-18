// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLNet.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLConstructorException.h"

using namespace naja::SNL;

TEST(SNLVRLConstructorTest0, test0) {
  EXPECT_EQ(SNLNet::Type::Standard,
    SNLVRLConstructor::VRLTypeToSNLType(naja::verilog::Net::Type::Wire));
  EXPECT_EQ(SNLNet::Type::Supply0,
    SNLVRLConstructor::VRLTypeToSNLType(naja::verilog::Net::Type::Supply0));
  EXPECT_EQ(SNLNet::Type::Supply1,
    SNLVRLConstructor::VRLTypeToSNLType(naja::verilog::Net::Type::Supply1));
  EXPECT_THROW(
    SNLVRLConstructor::VRLTypeToSNLType(naja::verilog::Net::Type::Unknown),
    SNLVRLConstructorException
  );
}

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