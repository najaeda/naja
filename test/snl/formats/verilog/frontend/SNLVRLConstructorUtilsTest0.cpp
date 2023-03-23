#include "gtest/gtest.h"

#include "SNLVRLConstructorUtils.h"

using namespace naja::SNL;

TEST(SNLVRLConstructorUtilsTest0, testDecimal) {
  auto num = naja::verilog::BasedNumber("4", "d", "2");
  auto bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_FALSE(bits[3]);
  EXPECT_FALSE(bits[2]);
  EXPECT_TRUE(bits[1]);
  EXPECT_FALSE(bits[0]);

  num = naja::verilog::BasedNumber("4", "d", "2");
  bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_FALSE(bits[3]);
  EXPECT_FALSE(bits[2]);
  EXPECT_TRUE(bits[1]);
  EXPECT_FALSE(bits[0]);

  std::string decimalStr("1073746176");
  std::string hexaStr("40001100");

  auto numDecimal = naja::verilog::BasedNumber("32", "d", decimalStr);
  auto numHexa = naja::verilog::BasedNumber("32", "h", hexaStr);
  auto bitsDecimal = SNLVRLConstructorUtils::numberToBits(numDecimal);
  auto bitsHexa = SNLVRLConstructorUtils::numberToBits(numHexa);
  EXPECT_EQ(bitsDecimal, bitsHexa);
}

TEST(SNLVRLConstructorUtilsTest0, testHexadecimal) {
  for (size_t i=0; i<=15; i++) {
    std::stringstream stream;
    stream << std::hex << i;
    std::string hexStr = stream.str();
    auto num = naja::verilog::BasedNumber("4", "h", hexStr);
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

  auto num = naja::verilog::BasedNumber("2", "h", "D");
  auto bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_FALSE(bits[3]); //truncated 
  EXPECT_FALSE(bits[2]); //truncated
  EXPECT_FALSE(bits[1]);
  EXPECT_TRUE(bits[0]);

  num = naja::verilog::BasedNumber("6", "h", "2D");
  bits = SNLVRLConstructorUtils::numberToBits(num);
  EXPECT_TRUE(bits[5]);
  EXPECT_FALSE(bits[4]);
  EXPECT_TRUE(bits[3]);
  EXPECT_TRUE(bits[2]);
  EXPECT_FALSE(bits[1]);
  EXPECT_TRUE(bits[0]);
}