
// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLBitDependencies.h"
#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLException.h"
#include "SNLDesignModeling.h"
using namespace naja::NL;

class SNLGateTruthTableTest: public ::testing::Test {
  protected:
    std::vector<size_t> getInputFlatTermPositions(const SNLDesign* design) {
      std::vector<size_t> positions;
      size_t flatPos = 0;
      for (auto term : design->getBitTerms()) {
        if (term->getDirection() == SNLTerm::Direction::Input) {
          positions.push_back(flatPos);
        }
        ++flatPos;
      }
      return positions;
    }

    void expectGenericGate(const SNLDesign* design,
                           const SNLTruthTable& tt,
                           uint32_t size,
                           SNLTruthTable::GenericType type) {
      EXPECT_EQ(tt.size(), size);
      EXPECT_TRUE(tt.isGeneric());
      EXPECT_EQ(tt.getGenericType(), type);
      const auto deps = NLBitDependencies::decodeBits(tt.getDependencies());
      EXPECT_EQ(deps, getInputFlatTermPositions(design));
    }

    void expectUnaryGate(const SNLDesign* design,
                         const SNLTruthTable& tt,
                         uint64_t expectedBits) {
      EXPECT_EQ(tt.size(), 1);
      EXPECT_FALSE(tt.isGeneric());
      EXPECT_EQ(static_cast<uint64_t>(tt.bits()), expectedBits);
      const auto deps = NLBitDependencies::decodeBits(tt.getDependencies());
      EXPECT_EQ(deps, getInputFlatTermPositions(design));
    }

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
    expectGenericGate(and2, tt, 2, SNLTruthTable::GenericType::AND);

    auto and4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 4);
    ASSERT_NE(and4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(and4);
    expectGenericGate(and4, tt, 4, SNLTruthTable::GenericType::AND);

    auto and5 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 5);
    ASSERT_NE(and5, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(and5);
    expectGenericGate(and5, tt, 5, SNLTruthTable::GenericType::AND);
}

TEST_F(SNLGateTruthTableTest, testNandGate) {
    auto nand2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Nand, 2);
    ASSERT_NE(nand2, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(nand2);
    expectGenericGate(nand2, tt, 2, SNLTruthTable::GenericType::NAND);

    auto nand6 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Nand, 6);
    ASSERT_NE(nand6, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(nand6);
    expectGenericGate(nand6, tt, 6, SNLTruthTable::GenericType::NAND);
}

TEST_F(SNLGateTruthTableTest, testOrGate) {
    auto or2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 2);
    ASSERT_NE(or2, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(or2);
    expectGenericGate(or2, tt, 2, SNLTruthTable::GenericType::OR);

    auto or4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 4);
    ASSERT_NE(or4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(or4);
    expectGenericGate(or4, tt, 4, SNLTruthTable::GenericType::OR);

    auto or5 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 5);
    ASSERT_NE(or5, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(or5);
    expectGenericGate(or5, tt, 5, SNLTruthTable::GenericType::OR);
    EXPECT_EQ(SNLDesignModeling::getTruthTable(or5), tt);

    auto or6 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 6);
    ASSERT_NE(or6, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(or6);
    expectGenericGate(or6, tt, 6, SNLTruthTable::GenericType::OR);
    EXPECT_EQ(SNLDesignModeling::getTruthTable(or6), tt);
}

TEST_F(SNLGateTruthTableTest, testNorGate) {
    auto nor = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Nor, 2);
    ASSERT_NE(nor, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(nor);
    expectGenericGate(nor, tt, 2, SNLTruthTable::GenericType::NOR);

    auto nor3 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Nor, 3);
    ASSERT_NE(nor3, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(nor3);
    expectGenericGate(nor3, tt, 3, SNLTruthTable::GenericType::NOR);

    auto nor4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Nor, 4);
    ASSERT_NE(nor4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(nor4);
    expectGenericGate(nor4, tt, 4, SNLTruthTable::GenericType::NOR);
}

TEST_F(SNLGateTruthTableTest, testXorGate) {
    auto xor2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2);
    ASSERT_NE(xor2, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(xor2);
    expectGenericGate(xor2, tt, 2, SNLTruthTable::GenericType::XOR);

    auto xor3 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 3);
    ASSERT_NE(xor3, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(xor3);
    expectGenericGate(xor3, tt, 3, SNLTruthTable::GenericType::XOR);

    auto xor4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 4);
    ASSERT_NE(xor4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(xor4);
    expectGenericGate(xor4, tt, 4, SNLTruthTable::GenericType::XOR);
}

TEST_F(SNLGateTruthTableTest, testXnorGate) {
    auto xnor2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xnor, 2);
    ASSERT_NE(xnor2, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(xnor2);
    expectGenericGate(xnor2, tt, 2, SNLTruthTable::GenericType::XNOR);

    auto xnor3 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xnor, 3);
    ASSERT_NE(xnor3, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(xnor3);
    expectGenericGate(xnor3, tt, 3, SNLTruthTable::GenericType::XNOR);

    auto xnor4 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xnor, 4);
    ASSERT_NE(xnor4, nullptr);
    tt = NLDB0::getPrimitiveTruthTable(xnor4);
    expectGenericGate(xnor4, tt, 4, SNLTruthTable::GenericType::XNOR);
}

TEST_F(SNLGateTruthTableTest, testUnaryGates) {
    auto buf = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 1);
    ASSERT_NE(buf, nullptr);
    expectUnaryGate(buf, NLDB0::getPrimitiveTruthTable(buf), 0b10);
    expectUnaryGate(buf, SNLDesignModeling::getTruthTable(buf), 0b10);
    EXPECT_TRUE(SNLDesignModeling::isBuf(buf));

    auto inv = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1);
    ASSERT_NE(inv, nullptr);
    expectUnaryGate(inv, NLDB0::getPrimitiveTruthTable(inv), 0b01);
    expectUnaryGate(inv, SNLDesignModeling::getTruthTable(inv), 0b01);
    EXPECT_TRUE(SNLDesignModeling::isInv(inv));
}

TEST_F(SNLGateTruthTableTest, testUnsupportedGates) {
    auto andGate = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 10);
    ASSERT_NE(andGate, nullptr);
    expectGenericGate(
        andGate, NLDB0::getPrimitiveTruthTable(andGate), 10,
        SNLTruthTable::GenericType::AND);

    auto bufGate = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 2);
    ASSERT_NE(bufGate, nullptr);
    EXPECT_THROW(NLDB0::getPrimitiveTruthTable(bufGate), NLException);
}

TEST_F(SNLGateTruthTableTest, testAssignTruthTable) {
    auto assign = NLDB0::getAssign();
    ASSERT_NE(assign, nullptr);
    auto tt = NLDB0::getPrimitiveTruthTable(assign);
    EXPECT_EQ(SNLTruthTable::Buf(), tt);
}

TEST_F(SNLGateTruthTableTest, testFATruthTables) {
    auto fa = NLDB0::getFA();
    ASSERT_NE(nullptr, fa);

    // Sum = A XOR B XOR CI: same truth table as XOR-3
    auto sumTT = NLDB0::getFASumTruthTable();
    EXPECT_EQ(3, sumTT.size());
    EXPECT_EQ(0x96ULL, static_cast<uint64_t>(sumTT.bits()));

    // Cross-check: Sum truth table equals the XOR-3 truth table
    auto xor3 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 3);
    ASSERT_NE(nullptr, xor3);
    expectGenericGate(
        xor3, NLDB0::getPrimitiveTruthTable(xor3), 3,
        SNLTruthTable::GenericType::XOR);

    // Cout = majority(A,B,CI): output 1 when at least 2 inputs are 1
    auto coutTT = NLDB0::getFACoutTruthTable();
    EXPECT_EQ(3, coutTT.size());
    EXPECT_EQ(0xE8ULL, static_cast<uint64_t>(coutTT.bits()));

    // Verify majority row by row: Cout=1 iff popcount(i)>=2
    for (uint64_t i = 0; i < 8; ++i) {
        bool expected = (__builtin_popcountll(i) >= 2);
        bool actual   = ((coutTT.bits() >> i) & 1ULL) != 0;
        EXPECT_EQ(expected, actual) << "row " << i;
    }

    // getPrimitiveTruthTable must throw for FA (two outputs)
    EXPECT_THROW(NLDB0::getPrimitiveTruthTable(fa), NLException);
}
