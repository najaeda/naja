#include "gtest/gtest.h"

#include "SNLNet.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLConstructorException.h"

using namespace naja::SNL;

TEST(SNLVRLConstructorTest0, test) {
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