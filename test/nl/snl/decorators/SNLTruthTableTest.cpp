// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLTruthTable.h"
using namespace naja::NL;

TEST(SNLTruthTableTest, test) {

  SNLTruthTable ttand2(2, 0b1000);
  EXPECT_EQ(2, ttand2.size());
  EXPECT_EQ(0b1000, ttand2.bits());
  EXPECT_FALSE(ttand2.all0());
  EXPECT_FALSE(ttand2.all1());
  auto reducedtt = ttand2.getReducedWithConstant(0, 0);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all0());
  reducedtt = ttand2.getReducedWithConstant(0, 1);
  EXPECT_EQ(reducedtt.size(), 1);
  //buffer
  EXPECT_EQ(0b10, reducedtt.bits());
  SNLTruthTable ttor2(2, 0b1110);
  reducedtt = ttor2.getReducedWithConstant(0, 0);
  EXPECT_EQ(reducedtt.size(), 1);
  //buffer
  EXPECT_EQ(0b10, reducedtt.bits());
  reducedtt = ttor2.getReducedWithConstant(0, 1);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all1());

  SNLTruthTable ttand4(4, 0b1000000000000000);
  reducedtt = ttand4.getReducedWithConstant(0, 0);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all0());
  reducedtt = ttand4.getReducedWithConstant(0, 1);
  EXPECT_EQ(reducedtt.size(), 3);
  //and3
  EXPECT_EQ(0b10000000, reducedtt.bits());
  EXPECT_EQ(reducedtt, SNLTruthTable(3, 0b10000000));
  
  reducedtt = ttand4.getReducedWithConstant(1, 0);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all0());

  reducedtt = ttand4.getReducedWithConstant(1, 1);
  //and3
  EXPECT_EQ(0b10000000, reducedtt.bits());
  EXPECT_EQ(reducedtt, SNLTruthTable(3, 0b10000000));

  SNLTruthTable ttor4(4, 0b1111111111111110);
  reducedtt = ttor4.getReducedWithConstant(0, 1);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all1());

  reducedtt = ttor4.getReducedWithConstant(0, 0);
  //or3
  EXPECT_EQ(0b11111110, reducedtt.bits());
  EXPECT_EQ(reducedtt, SNLTruthTable(3, 0b11111110));

  SNLTruthTable ttxor2(2, 0b0110);
  reducedtt = ttxor2.getReducedWithConstant(0, 0);
  //buffer
  EXPECT_EQ(0b10, reducedtt.bits());

  reducedtt = ttxor2.getReducedWithConstant(0, 1);
  //invert
  EXPECT_EQ(0b01, reducedtt.bits());

  //function: "!(A | (B1 & B2))";
  //order 0: A 1: B1 2: B2
  SNLTruthTable tt(3, 0x15);
  
  //set A to 1 => Logic0
  reducedtt = tt.getReducedWithConstant(0, 1);
  //0
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_TRUE(reducedtt.all0());
  reducedtt = reducedtt.getReducedWithConstant(0, 0);
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_TRUE(reducedtt.all0());

  //set A to 0 => !(B1 & B2)
  reducedtt = tt.getReducedWithConstant(0, 0);
  //!(B1 & B2)
  EXPECT_EQ(2, reducedtt.size());
  EXPECT_EQ(0b0111, reducedtt.bits());

  //set B1 to 1
  reducedtt = tt.getReducedWithConstant(1, 1);
  //!(A | B2) nor
  EXPECT_EQ(2, reducedtt.size());
  EXPECT_EQ(0x1, reducedtt.bits());

  //set B1 to 0
  reducedtt = tt.getReducedWithConstant(1, 0);
  EXPECT_EQ(2, reducedtt.size());
  EXPECT_EQ(0x5, reducedtt.bits());

  //B2 has no influence
  EXPECT_TRUE(reducedtt.hasNoInfluence(1));
  EXPECT_FALSE(reducedtt.hasNoInfluence(0));

  //remove B2 (index 1)
  reducedtt = reducedtt.removeVariable(1);
  EXPECT_EQ(1, reducedtt.size());
  //gate is now !A
  EXPECT_EQ(0x1, reducedtt.bits());
}

TEST(SNLTruthTableTest, testConstants) {
  SNLTruthTable tt0(0, 0);
  EXPECT_TRUE(tt0.all0());
  EXPECT_FALSE(tt0.all1());
  EXPECT_EQ(tt0, SNLTruthTable::Logic0());

  SNLTruthTable tt1(0, 1);
  EXPECT_FALSE(tt1.all0());
  EXPECT_TRUE(tt1.all1());
  EXPECT_EQ(tt1, SNLTruthTable::Logic1());

  SNLTruthTable tt2(4, 0b11); //not all one
  EXPECT_FALSE(tt2.all1());
  EXPECT_FALSE(tt2.all0());
  EXPECT_NE(tt2, SNLTruthTable::Logic1());
}

TEST(SNLTruthTable, testMultipleConstantInputs) {
  //mux truth table
  //function		: "((S & B) | (A & !S))";
  SNLTruthTable tt(3, 0xCA);

  //A&B are 0
  auto reducedtt = tt.getReducedWithConstants({{0, 0}, {1, 0}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x0, reducedtt.bits());

  //A&B are 1
  reducedtt = tt.getReducedWithConstants({{0, 1}, {1, 1}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x1, reducedtt.bits());

  //S=0 B=0 => A
  reducedtt = tt.getReducedWithConstants({{1, 0}, {2, 0}});
  EXPECT_EQ(1, reducedtt.size());
  EXPECT_EQ(0x2, reducedtt.bits());

  //S=0 B=1 => A
  reducedtt = tt.getReducedWithConstants({{1, 1}, {2, 0}});
  EXPECT_EQ(1, reducedtt.size());
  EXPECT_EQ(0x2, reducedtt.bits());

  //S=1 B=0 => 0
  reducedtt = tt.getReducedWithConstants({{1, 0}, {2, 1}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x0, reducedtt.bits());

  //S=1 B=1 => 1
  reducedtt = tt.getReducedWithConstants({{1, 1}, {2, 1}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x1, reducedtt.bits());

  //S=0 A=0 => 0
  reducedtt = tt.getReducedWithConstants({{0, 0}, {2, 0}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x0, reducedtt.bits());

  //S=0 A=1 => 1
  reducedtt = tt.getReducedWithConstants({{0, 1}, {2, 0}});
  EXPECT_EQ(0, reducedtt.size());

  //A=0, B=1 => S
  reducedtt = tt.getReducedWithConstants({{0, 0}, {1, 1}});
  EXPECT_EQ(1, reducedtt.size());
  EXPECT_EQ(0x2, reducedtt.bits());

  //A=1, B=0 => !S
  reducedtt = tt.getReducedWithConstants({{0, 1}, {1, 0}});
  EXPECT_EQ(1, reducedtt.size());
  EXPECT_EQ(0x1, reducedtt.bits());
}

TEST(SNLTruthTable, testErrors) {
  EXPECT_THROW(SNLTruthTable(8, 0xFFFFF), NLException);
  SNLTruthTable tt(4, 0xFFFF);
  EXPECT_THROW(tt.getReducedWithConstant(5, 0), NLException);
  EXPECT_THROW(tt.removeVariable(5), NLException);
}
