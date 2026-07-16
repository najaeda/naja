// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLException.h"
#include "NLLogicValue.h"

using namespace naja::NL;

TEST(NLLogicVectorTest, FilledAndBitAccess) {
  auto value = NLLogicVector::filled(70, NLLogicValue::X);
  EXPECT_EQ(70, value.getWidth());
  EXPECT_TRUE(value.isAll(NLLogicValue::X));
  EXPECT_TRUE(value.containsX());
  EXPECT_FALSE(value.containsZ());
  EXPECT_FALSE(value.isTwoState());

  value.setBit(0, NLLogicValue::Zero);
  value.setBit(63, NLLogicValue::One);
  value.setBit(69, NLLogicValue::Z);
  EXPECT_EQ(NLLogicValue::Zero, value.getBit(0));
  EXPECT_EQ(NLLogicValue::One, value.getBit(63));
  EXPECT_EQ(NLLogicValue::Z, value.getBit(69));
  EXPECT_TRUE(value.containsX());
  EXPECT_TRUE(value.containsZ());
  EXPECT_THROW(value.getBit(70), NLException);
  EXPECT_THROW(value.setBit(70, NLLogicValue::Zero), NLException);
}

TEST(NLLogicVectorTest, VerilogBinaryRoundTrip) {
  auto value = NLLogicVector::fromVerilogBinary("8'b10XZ_0011");
  EXPECT_EQ(8, value.getWidth());
  EXPECT_EQ("10xz0011", value.toBinaryDigits());
  EXPECT_EQ("8'b10xz0011", value.toVerilogBinary());
  EXPECT_EQ(NLLogicValue::One, value.getBit(0));
  EXPECT_EQ(NLLogicValue::Zero, value.getBit(2));
  EXPECT_EQ(NLLogicValue::Z, value.getBit(4));
  EXPECT_EQ(NLLogicValue::X, value.getBit(5));
  EXPECT_EQ(value, NLLogicVector::fromVerilogBinary(value.toVerilogBinary()));
}

TEST(NLLogicVectorTest, KnownAndUniformValues) {
  auto zero = NLLogicVector::filled(65, NLLogicValue::Zero);
  auto one = NLLogicVector::filled(65, NLLogicValue::One);
  EXPECT_TRUE(zero.isTwoState());
  EXPECT_TRUE(zero.isFullyKnown());
  EXPECT_TRUE(zero.isAll(NLLogicValue::Zero));
  EXPECT_TRUE(one.isAll(NLLogicValue::One));
  EXPECT_FALSE(zero.containsX());
  EXPECT_FALSE(zero.containsZ());
  EXPECT_NE(zero, one);
}

TEST(NLLogicVectorTest, InvalidValues) {
  EXPECT_THROW(NLLogicVector::filled(0, NLLogicValue::Zero), NLException);
  EXPECT_THROW(NLLogicVector::fromVerilogBinary(""), NLException);
  EXPECT_THROW(NLLogicVector::fromVerilogBinary("0'b"), NLException);
  EXPECT_THROW(NLLogicVector::fromVerilogBinary("4'hf"), NLException);
  EXPECT_THROW(NLLogicVector::fromVerilogBinary("4'b01"), NLException);
  EXPECT_THROW(NLLogicVector::fromVerilogBinary("2'b0q"), NLException);
  EXPECT_THROW(NLLogicVector::fromVerilogBinary("4'b_1010"), NLException);
  EXPECT_THROW(NLLogicVector::fromVerilogBinary("4'b10__10"), NLException);
  EXPECT_THROW(NLLogicVector::fromVerilogBinary("4'b1010_"), NLException);
}

TEST(NLLogicVectorTest, EmptyValueIsNotAConstant) {
  NLLogicVector value;
  EXPECT_TRUE(value.empty());
  EXPECT_EQ(0, value.getWidth());
  EXPECT_FALSE(value.isAll(NLLogicValue::Zero));
  EXPECT_EQ("", value.toBinaryDigits());
  EXPECT_THROW(value.toVerilogBinary(), NLException);
}
