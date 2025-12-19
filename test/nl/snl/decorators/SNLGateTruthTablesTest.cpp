
// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLDB0.h"
using namespace naja::NL;

class SNLGateTruthTableTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLGateTruthTableTest, testAndGate) {
    auto and2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
    ASSERT_NE(and2, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(and2);
    EXPECT_EQ(tt.size(), 2);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0b1000);

    auto and4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 4);
    ASSERT_NE(and4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(and4);
    EXPECT_EQ(tt.size(), 4);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0x8000);

    auto and5 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 5);
    ASSERT_NE(and5, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(and5);
    EXPECT_EQ(tt.size(), 5);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0x80000000);
}

TEST_F(SNLGateTruthTableTest, testOrGate) {
    auto or2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 2);
    ASSERT_NE(or2, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(or2);
    EXPECT_EQ(tt.size(), 2);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0b1110);

    auto or4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 4);
    ASSERT_NE(or4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(or4);
    EXPECT_EQ(tt.size(), 4);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0xFFFE);

    auto or5 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 5);
    ASSERT_NE(or5, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(or5);
    EXPECT_EQ(tt.size(), 5);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0xFFFFFFFE);
}

TEST_F(SNLGateTruthTableTest, testNorGate) {
    auto nor = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Nor, 2);
    ASSERT_NE(nor, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(nor);
    EXPECT_EQ(tt.size(), 2);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0b0001);

    auto nor3 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Nor, 3);
    ASSERT_NE(nor3, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(nor3);
    EXPECT_EQ(tt.size(), 3);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0x01);

    auto nor4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Nor, 4);
    ASSERT_NE(nor4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(nor4);
    EXPECT_EQ(tt.size(), 4);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0x0001);
}

TEST_F(SNLGateTruthTableTest, testXorGate) {
    auto xor2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2);
    ASSERT_NE(xor2, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(xor2);
    EXPECT_EQ(tt.size(), 2);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0x6);

    auto xor3 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 3);
    ASSERT_NE(xor3, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(xor3);
    EXPECT_EQ(tt.size(), 3);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0x96);

    auto xor4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 4);
    ASSERT_NE(xor4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(xor4);
    EXPECT_EQ(tt.size(), 4);
    EXPECT_EQ(static_cast<uint64_t>(tt.bits()), 0x6996);
}

TEST_F(SNLGateTruthTableTest, testUnsupportedGates) {
    auto andGate = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 10);
    ASSERT_NE(andGate, nullptr);
    EXPECT_THROW(NLDB0::getPrimitiveTruthTable(andGate), NLException);
}