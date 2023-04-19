#include "gtest/gtest.h"

#include "SNLVRLConstructorUtils.h"
#include "SNLVRLConstructorException.h"

using namespace naja::SNL;

TEST(SNLVRLConstructorUtilsTest0, testBinary) {
  auto num = naja::verilog::BasedNumber("4", false, 'b', "1010");
  auto bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_FALSE(num.signed_);
  EXPECT_EQ(4, bits.size());
  EXPECT_FALSE(bits[3]);
  EXPECT_TRUE(bits[2]);
  EXPECT_FALSE(bits[1]);
  EXPECT_TRUE(bits[0]);
}

TEST(SNLVRLConstructorUtilsTest0, testDecimal) {
  auto num = naja::verilog::BasedNumber("4", true, 'd', "2");
  auto bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_EQ(4, bits.size());
  EXPECT_FALSE(bits[3]);
  EXPECT_FALSE(bits[2]);
  EXPECT_TRUE(bits[1]);
  EXPECT_FALSE(bits[0]);

  num = naja::verilog::BasedNumber("4", true, 'd', "2");
  bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_TRUE(num.signed_);
  EXPECT_EQ(4, bits.size());
  EXPECT_FALSE(bits[3]);
  EXPECT_FALSE(bits[2]);
  EXPECT_TRUE(bits[1]);
  EXPECT_FALSE(bits[0]);

  std::string decimalStr("1073746176");
  std::string hexaStr("40001100");
  auto numDecimal = naja::verilog::BasedNumber("32", false, 'd', decimalStr);
  auto numHexa = naja::verilog::BasedNumber("32", false, 'h', hexaStr);
  EXPECT_FALSE(numDecimal.signed_);
  EXPECT_FALSE(numHexa.signed_);
  auto bitsDecimal = SNLVRLConstructorUtils::numberToBits(numDecimal);
  auto bitsHexa = SNLVRLConstructorUtils::numberToBits(numHexa);
  EXPECT_EQ(32, bitsDecimal.size());
  EXPECT_EQ(32, bitsHexa.size());
  EXPECT_EQ(bitsDecimal, bitsHexa);
}

TEST(SNLVRLConstructorUtilsTest0, testHexadecimal) {
  for (size_t i=0; i<=15; i++) {
    std::stringstream stream;
    stream << std::hex << i;
    std::string hexStr = stream.str();
    auto num = naja::verilog::BasedNumber("4", false, 'h', hexStr);
    auto bits = SNLVRLConstructorUtils::numberToBits(num);
    boost::dynamic_bitset<> ref(4, 0ul); 
    int intNum = i;
    int j = 0;
    while (intNum > 0) {
      ref[j] = intNum % 2;
		  j++;
		  intNum = intNum / 2;
    }
    EXPECT_EQ(ref, bits);
  }

  auto num = naja::verilog::BasedNumber("2", true, 'h', "D");
  auto bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_EQ(2, bits.size());
  EXPECT_FALSE(bits[1]);
  EXPECT_TRUE(bits[0]);

  num = naja::verilog::BasedNumber("6", true, 'h', "2D");
  EXPECT_TRUE(num.signed_);
  bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_EQ(6, bits.size());
  EXPECT_TRUE(bits[5]);
  EXPECT_FALSE(bits[4]);
  EXPECT_TRUE(bits[3]);
  EXPECT_TRUE(bits[2]);
  EXPECT_FALSE(bits[1]);
  EXPECT_TRUE(bits[0]);
}

TEST(SNLVRLConstructorUtilsTest0, testErrors) {
  {
    //Octal not supported for the moment
    auto num = naja::verilog::BasedNumber("3", false, 'o', "7");
    EXPECT_THROW(SNLVRLConstructorUtils::numberToBits(num), SNLVRLConstructorException);
  }

  {
    //Wrong characters for binary
    auto num = naja::verilog::BasedNumber("3", false, 'b', "1A0");
    EXPECT_THROW(SNLVRLConstructorUtils::numberToBits(num), SNLVRLConstructorException);
  }

  {
    //Wrong characters for hexa
    auto num = naja::verilog::BasedNumber("5", false, 'h', "1234Z");
    EXPECT_THROW(SNLVRLConstructorUtils::numberToBits(num), SNLVRLConstructorException);
  }
}
