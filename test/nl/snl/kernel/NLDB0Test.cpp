// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLBitDependencies.h"
#include "NLLibraryTruthTables.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesignModeling.h"
#include "NLException.h"
#include <algorithm>
using namespace naja::NL;

namespace {

template <typename T>
std::vector<T*> collectTerms(const naja::NajaCollection<T*>& collection) {
  return std::vector<T*>(collection.begin(), collection.end());
}

template <typename T>
void expectTerms(const std::vector<T*>& actual, const std::vector<T*>& expected) {
  EXPECT_EQ(expected.size(), actual.size());
  for (auto* term: expected) {
    EXPECT_NE(actual.end(), std::find(actual.begin(), actual.end(), term));
  }
}

struct SequentialPrimitiveData {
  SNLDesign* design {};
  SNLScalarTerm* clock {};
  std::vector<SNLBitTerm*> inputs;
  std::vector<SNLBitTerm*> outputs;
};

}  // namespace

class NLDB0Test: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(NLDB0Test, testAssign) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto db0 = NLUniverse::get()->getDB(0);
  ASSERT_NE(nullptr, db0);
  EXPECT_TRUE(NLUniverse::isDB0(db0));
  EXPECT_EQ(NLDB0::getDB0(), db0);
  EXPECT_EQ(NLUniverse::getDB0(), db0);

  auto assign = NLDB0::getAssign();
  ASSERT_NE(nullptr, assign);
  EXPECT_TRUE(NLDB0::isDB0Primitive(assign));
  EXPECT_EQ(0, assign->getID());
  auto assignInput = NLDB0::getAssignInput();
  ASSERT_NE(nullptr, assignInput);
  EXPECT_EQ(0, assignInput->getID());
  auto assignOutput = NLDB0::getAssignOutput();
  ASSERT_NE(nullptr, assignOutput);
  EXPECT_EQ(1, assignOutput->getID());
  SNLBitNet* assignFT = assignInput->getNet();
  ASSERT_NE(nullptr, assignFT);
  EXPECT_EQ(assignOutput->getNet(), assignFT);

  auto db1 = NLDB::create(NLUniverse::get());
  EXPECT_FALSE(NLUniverse::isDB0(db1));

  //EXPECT_EQ(nullptr, NLDB0::getANDOutput(assign));
  //EXPECT_EQ(nullptr, NLDB0::getANDInputs(assign));

  EXPECT_FALSE(NLDB0::getDB0()->isTopDB());
  EXPECT_THROW(NLUniverse::get()->setTopDB(db0), NLException);
}

TEST_F(NLDB0Test, testAND) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  SNLDesign* and2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
  ASSERT_NE(nullptr, and2);
  SNLScalarTerm* and2Out = NLDB0::getGateSingleTerm(and2);
  ASSERT_NE(nullptr, and2Out);
  EXPECT_EQ(NLID::DesignObjectID(0), and2Out->getID());
  EXPECT_EQ(SNLTerm::Direction::Output, and2Out->getDirection());
  SNLBusTerm* and2Inputs = NLDB0::getGateNTerms(and2);
  ASSERT_NE(nullptr, and2Inputs);
  EXPECT_EQ(NLID::DesignObjectID(1), and2Inputs->getID());
  EXPECT_EQ(SNLTerm::Direction::Input, and2Inputs->getDirection());
  EXPECT_EQ(2, and2Inputs->getWidth());

  SNLDesign* and2Test = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
  EXPECT_EQ(and2, and2Test);

  SNLDesign* and48 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 48);
  ASSERT_NE(nullptr, and48);
  SNLScalarTerm* and48Out = NLDB0::getGateSingleTerm(and48);
  ASSERT_NE(nullptr, and48Out);
  EXPECT_EQ(NLID::DesignObjectID(0), and48Out->getID());
  EXPECT_EQ(SNLTerm::Direction::Output, and48Out->getDirection());
  SNLBusTerm* and48Inputs = NLDB0::getGateNTerms(and48);
  ASSERT_NE(nullptr, and48Inputs);
  EXPECT_EQ(NLID::DesignObjectID(1), and48Inputs->getID());
  EXPECT_EQ(SNLTerm::Direction::Input, and48Inputs->getDirection());
  EXPECT_EQ(48, and48Inputs->getWidth());
}

TEST_F(NLDB0Test, testFA) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto fa = NLDB0::getFA();
  ASSERT_NE(nullptr, fa);
  EXPECT_TRUE(NLDB0::isFA(fa));
  EXPECT_TRUE(NLDB0::isDB0Primitive(fa));
  EXPECT_EQ(NLName("naja_fa"), fa->getName());
  EXPECT_FALSE(NLDB0::isAssign(fa));
  EXPECT_FALSE(NLDB0::isMux2(fa));

  auto inA = NLDB0::getFAInputA();
  ASSERT_NE(nullptr, inA);
  EXPECT_EQ(NLName("A"), inA->getName());
  EXPECT_EQ(SNLTerm::Direction::Input, inA->getDirection());

  auto inB = NLDB0::getFAInputB();
  ASSERT_NE(nullptr, inB);
  EXPECT_EQ(NLName("B"), inB->getName());
  EXPECT_EQ(SNLTerm::Direction::Input, inB->getDirection());

  auto inCI = NLDB0::getFAInputCI();
  ASSERT_NE(nullptr, inCI);
  EXPECT_EQ(NLName("CI"), inCI->getName());
  EXPECT_EQ(SNLTerm::Direction::Input, inCI->getDirection());

  auto outS = NLDB0::getFAOutputS();
  ASSERT_NE(nullptr, outS);
  EXPECT_EQ(NLName("S"), outS->getName());
  EXPECT_EQ(SNLTerm::Direction::Output, outS->getDirection());

  auto outCO = NLDB0::getFAOutputCO();
  ASSERT_NE(nullptr, outCO);
  EXPECT_EQ(NLName("CO"), outCO->getName());
  EXPECT_EQ(SNLTerm::Direction::Output, outCO->getDirection());

  // all 5 terminals belong to the same design
  EXPECT_EQ(fa, inA->getDesign());
  EXPECT_EQ(fa, inB->getDesign());
  EXPECT_EQ(fa, inCI->getDesign());
  EXPECT_EQ(fa, outS->getDesign());
  EXPECT_EQ(fa, outCO->getDesign());

  // getPrimitiveTruthTable must throw for FA (two outputs)
  EXPECT_THROW(NLDB0::getPrimitiveTruthTable(fa), NLException);
}

TEST_F(NLDB0Test, testPrimitiveTruthTableFallbacksAndErrors) {
  auto sumWithoutDB0 = NLDB0::getFASumTruthTable();
  auto coutWithoutDB0 = NLDB0::getFACoutTruthTable();
  EXPECT_EQ(SNLTruthTable::fullDependencies(3), sumWithoutDB0.getDependencies());
  EXPECT_EQ(SNLTruthTable::fullDependencies(3), coutWithoutDB0.getDependencies());

  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto* fa = NLDB0::getFA();
  ASSERT_NE(nullptr, fa);
  EXPECT_THROW(
      SNLDesignModeling::getTruthTable(fa, NLDB0::getFAInputA()->getFlatID()),
      NLException);

  auto* mux2 = NLDB0::getMux2();
  ASSERT_NE(nullptr, mux2);
  EXPECT_THROW(
      SNLDesignModeling::getTruthTable(mux2, NLDB0::getMux2Select(mux2)->getFlatID()),
      NLException);

  auto* primitives = NLDB0::getDB0RootLibrary();
  ASSERT_NE(nullptr, primitives);
  auto* unsupported =
      SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("unsupported"));
  auto* unsupportedInput =
      SNLScalarTerm::create(unsupported, SNLTerm::Direction::Input, NLName("I"));
  auto* unsupportedOutput0 =
      SNLScalarTerm::create(unsupported, SNLTerm::Direction::Output, NLName("O0"));
  SNLScalarTerm::create(unsupported, SNLTerm::Direction::Output, NLName("O1"));

  EXPECT_TRUE(
      collectTerms(SNLDesignModeling::getInputRelatedClocks(unsupportedInput)).empty());
  EXPECT_THROW(NLDB0::getPrimitiveTruthTable(unsupported), NLException);
  EXPECT_THROW(
      SNLDesignModeling::getTruthTable(unsupported, unsupportedOutput0->getFlatID()),
      NLException);
}

TEST_F(NLDB0Test, testMux2TruthTable) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto mux2 = NLDB0::getMux2();
  ASSERT_NE(nullptr, mux2);
  EXPECT_TRUE(NLDB0::isMux2(mux2));
  EXPECT_EQ(NLName("naja_mux2__w1"), mux2->getName());
  auto* inA = NLDB0::getMux2InputA(mux2);
  auto* inB = NLDB0::getMux2InputB(mux2);
  auto* sel = NLDB0::getMux2Select(mux2);
  auto* out = NLDB0::getMux2Output(mux2);
  ASSERT_NE(nullptr, inA);
  ASSERT_NE(nullptr, inB);
  ASSERT_NE(nullptr, sel);
  ASSERT_NE(nullptr, out);
  EXPECT_EQ(1, inA->getWidth());
  EXPECT_EQ(1, inB->getWidth());
  EXPECT_EQ(1, out->getWidth());
  EXPECT_EQ(NLName("A"), inA->getName());
  EXPECT_EQ(NLName("B"), inB->getName());
  EXPECT_EQ(NLName("S"), sel->getName());
  EXPECT_EQ(NLName("Y"), out->getName());

  auto tt = NLDB0::getPrimitiveTruthTable(mux2);
  EXPECT_EQ(tt, SNLDesignModeling::getTruthTable(mux2));
  EXPECT_EQ(1u, SNLDesignModeling::getTruthTableCount(mux2));
  EXPECT_EQ(3u, tt.size());
  EXPECT_EQ(
    std::vector<size_t>({0, 1, 2}),
    NLBitDependencies::decodeBits(tt.getDependencies()));
  EXPECT_TRUE(SNLDesignModeling::areDependenciesDefined(inA->getBit(0)));
  EXPECT_TRUE(SNLDesignModeling::areDependenciesDefined(inB->getBit(0)));
  EXPECT_TRUE(SNLDesignModeling::areDependenciesDefined(sel));
  EXPECT_TRUE(SNLDesignModeling::areDependenciesDefined(out->getBit(0)));

  uint64_t bits = 0;
  for (uint64_t i = 0; i < (1ULL << tt.size()); ++i) {
    if (tt.bits().bit(i)) {
      bits |= (1ULL << i);
    }
  }
  // Truth table for Y = S ? B : A, with A/B/S mapped to input bits 0/1/2.
  EXPECT_EQ(0xCAULL, bits);
  for (uint64_t row = 0; row < (1ULL << tt.size()); ++row) {
    const bool a = row & 0x1ULL;
    const bool b = row & 0x2ULL;
    const bool s = row & 0x4ULL;
    EXPECT_EQ(s ? b : a, tt.bits().bit(row)) << "row " << row;
  }

  auto* mux232 = NLDB0::getOrCreateMux2(32);
  ASSERT_NE(nullptr, mux232);
  EXPECT_TRUE(NLDB0::isMux2(mux232));
  EXPECT_EQ(NLName("naja_mux2__w32"), mux232->getName());
  EXPECT_EQ(tt, NLDB0::getPrimitiveTruthTable(mux232));
  EXPECT_EQ(32u, SNLDesignModeling::getTruthTableCount(mux232));
  EXPECT_EQ(tt, SNLDesignModeling::getTruthTable(mux232));
  auto* mux232A = NLDB0::getMux2InputA(mux232);
  auto* mux232B = NLDB0::getMux2InputB(mux232);
  auto* mux232S = NLDB0::getMux2Select(mux232);
  auto* mux232Y = NLDB0::getMux2Output(mux232);
  ASSERT_NE(nullptr, mux232A);
  ASSERT_NE(nullptr, mux232B);
  ASSERT_NE(nullptr, mux232S);
  ASSERT_NE(nullptr, mux232Y);
  EXPECT_EQ(32, mux232A->getWidth());
  EXPECT_EQ(32, mux232B->getWidth());
  EXPECT_EQ(32, mux232Y->getWidth());

  for (auto* bitTerm: mux232Y->getBits()) {
    auto bitTT = SNLDesignModeling::getTruthTable(mux232, bitTerm->getFlatID());
    EXPECT_TRUE(bitTT.isInitialized());
    EXPECT_EQ(tt, bitTT);
    EXPECT_EQ(
      std::vector<size_t>({0, 1, 2}),
      NLBitDependencies::decodeBits(bitTT.getDependencies()));
  }

  auto truthTables = NLLibraryTruthTables::construct(NLDB0::getDB0RootLibrary());
  auto it = truthTables.find(tt);
  ASSERT_NE(truthTables.end(), it);
  EXPECT_NE(it->second.end(), std::find(it->second.begin(), it->second.end(), mux232));
}

TEST_F(NLDB0Test, testWideMux2ModelingArcs) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto* mux24 = NLDB0::getOrCreateMux2(4);
  ASSERT_NE(nullptr, mux24);

  auto* inA = NLDB0::getMux2InputA(mux24);
  auto* inB = NLDB0::getMux2InputB(mux24);
  auto* sel = NLDB0::getMux2Select(mux24);
  auto* out = NLDB0::getMux2Output(mux24);
  ASSERT_NE(nullptr, inA);
  ASSERT_NE(nullptr, inB);
  ASSERT_NE(nullptr, sel);
  ASSERT_NE(nullptr, out);

  for (size_t bit = 0; bit < out->getWidth(); ++bit) {
    auto* aBit = inA->getBit(bit);
    auto* bBit = inB->getBit(bit);
    auto* yBit = out->getBit(bit);
    ASSERT_NE(nullptr, aBit);
    ASSERT_NE(nullptr, bBit);
    ASSERT_NE(nullptr, yBit);

    auto muxInputs = std::vector(
      SNLDesignModeling::getCombinatorialInputs(yBit).begin(),
      SNLDesignModeling::getCombinatorialInputs(yBit).end());
    EXPECT_EQ(3u, muxInputs.size());
    EXPECT_NE(muxInputs.end(), std::find(muxInputs.begin(), muxInputs.end(), aBit));
    EXPECT_NE(muxInputs.end(), std::find(muxInputs.begin(), muxInputs.end(), bBit));
    EXPECT_NE(muxInputs.end(), std::find(muxInputs.begin(), muxInputs.end(), sel));
    EXPECT_EQ(
      muxInputs.end(),
      std::find(muxInputs.begin(), muxInputs.end(), inA->getBit((bit + 1) % out->getWidth())));
    EXPECT_EQ(
      muxInputs.end(),
      std::find(muxInputs.begin(), muxInputs.end(), inB->getBit((bit + 1) % out->getWidth())));

    auto fromA = std::vector(
      SNLDesignModeling::getCombinatorialOutputs(aBit).begin(),
      SNLDesignModeling::getCombinatorialOutputs(aBit).end());
    EXPECT_EQ(1u, fromA.size());
    EXPECT_EQ(yBit, fromA.front());

    auto fromB = std::vector(
      SNLDesignModeling::getCombinatorialOutputs(bBit).begin(),
      SNLDesignModeling::getCombinatorialOutputs(bBit).end());
    EXPECT_EQ(1u, fromB.size());
    EXPECT_EQ(yBit, fromB.front());
  }

  auto selectOutputs = std::vector(
    SNLDesignModeling::getCombinatorialOutputs(sel).begin(),
    SNLDesignModeling::getCombinatorialOutputs(sel).end());
  EXPECT_EQ(out->getWidth(), selectOutputs.size());
  for (size_t bit = 0; bit < out->getWidth(); ++bit) {
    EXPECT_NE(
      selectOutputs.end(),
      std::find(selectOutputs.begin(), selectOutputs.end(), out->getBit(bit)));
  }
}

TEST_F(NLDB0Test, testMemoryPrimitive) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  NLDB0::MemorySignature signature;
  signature.width = 8;
  signature.depth = 16;
  signature.abits = 4;
  signature.readPorts = 2;
  signature.writePorts = 3;
  signature.resetMode = NLDB0::MemoryResetMode::AsyncLow;

  auto* memory0 = NLDB0::getOrCreateMemory(signature);
  ASSERT_NE(nullptr, memory0);
  EXPECT_TRUE(memory0->isPrimitive());
  EXPECT_TRUE(NLDB0::isDB0Primitive(memory0));
  EXPECT_TRUE(NLDB0::isMemory(memory0));
  EXPECT_EQ(NLName("naja_mem__w8_d16_a4_r2_w3_rst_async_low"), memory0->getName());
  EXPECT_EQ(signature, NLDB0::getMemorySignature(memory0));

  auto* memory1 = NLDB0::getOrCreateMemory(signature);
  EXPECT_EQ(memory0, memory1);

  ASSERT_NE(nullptr, NLDB0::getMemoryClock(memory0));
  ASSERT_NE(nullptr, NLDB0::getMemoryReset(memory0));
  auto* raddr = NLDB0::getMemoryReadAddress(memory0);
  auto* rdata = NLDB0::getMemoryReadData(memory0);
  auto* waddr = NLDB0::getMemoryWriteAddress(memory0);
  auto* wdata = NLDB0::getMemoryWriteData(memory0);
  auto* we = NLDB0::getMemoryWriteEnable(memory0);
  ASSERT_NE(nullptr, raddr);
  ASSERT_NE(nullptr, rdata);
  ASSERT_NE(nullptr, waddr);
  ASSERT_NE(nullptr, wdata);
  ASSERT_NE(nullptr, we);
  EXPECT_EQ(8, raddr->getWidth());
  EXPECT_EQ(16, rdata->getWidth());
  EXPECT_EQ(12, waddr->getWidth());
  EXPECT_EQ(24, wdata->getWidth());
  EXPECT_EQ(3, we->getWidth());

  EXPECT_EQ("8", memory0->getParameter(NLName("WIDTH"))->getValue());
  EXPECT_EQ("16", memory0->getParameter(NLName("DEPTH"))->getValue());
  EXPECT_EQ("4", memory0->getParameter(NLName("ABITS"))->getValue());
  EXPECT_EQ("2", memory0->getParameter(NLName("RD_PORTS"))->getValue());
  EXPECT_EQ("3", memory0->getParameter(NLName("WR_PORTS"))->getValue());
  EXPECT_EQ("1", memory0->getParameter(NLName("RST_ENABLE"))->getValue());
  EXPECT_EQ("1", memory0->getParameter(NLName("RST_ASYNC"))->getValue());
  EXPECT_EQ("1", memory0->getParameter(NLName("RST_ACTIVE_LOW"))->getValue());
  EXPECT_EQ("1'b0", memory0->getParameter(NLName("INIT"))->getValue());

  EXPECT_THROW(NLDB0::getPrimitiveTruthTable(memory0), NLException);
  EXPECT_TRUE(SNLDesignModeling::hasModeling(memory0));
  EXPECT_TRUE(SNLDesignModeling::isSequential(memory0));

  auto* clk = NLDB0::getMemoryClock(memory0);
  auto* rst = NLDB0::getMemoryReset(memory0);
  ASSERT_NE(nullptr, clk);
  ASSERT_NE(nullptr, rst);

  auto clockOutputs = SNLDesignModeling::getClockRelatedOutputs(clk);
  EXPECT_EQ(rdata->getWidth(), clockOutputs.size());

  auto clockInputs = SNLDesignModeling::getClockRelatedInputs(clk);
  EXPECT_EQ(waddr->getWidth() + wdata->getWidth() + we->getWidth() + 1, clockInputs.size());

  auto readInputs = SNLDesignModeling::getCombinatorialInputs(
    static_cast<SNLBitTerm*>(rdata->getBit(0)));
  EXPECT_EQ(raddr->getWidth() / 2, readInputs.size());
}

TEST_F(NLDB0Test, testMemoryInstanceOverridesAreVisibleInSignature) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto* db = NLDB0::getDB0();
  ASSERT_NE(nullptr, db);
  auto* topLibrary = NLLibrary::create(db, NLName("WORK"));
  auto* top = SNLDesign::create(topLibrary, SNLDesign::Type::Standard, NLName("top"));
  NLDB0::MemorySignature signatureTemplate;
  signatureTemplate.width = 4;
  signatureTemplate.depth = 8;
  signatureTemplate.abits = 3;
  signatureTemplate.readPorts = 1;
  signatureTemplate.writePorts = 1;
  signatureTemplate.resetMode = NLDB0::MemoryResetMode::None;
  auto* memory = NLDB0::getOrCreateMemory(signatureTemplate);
  ASSERT_NE(nullptr, memory);

  auto* inst = SNLInstance::create(top, memory, NLName("mem0"));
  ASSERT_NE(nullptr, inst);
  SNLInstParameter::create(inst, memory->getParameter(NLName("WIDTH")), "7");
  SNLInstParameter::create(inst, memory->getParameter(NLName("DEPTH")), "4");
  SNLInstParameter::create(inst, memory->getParameter(NLName("ABITS")), "2");
  SNLInstParameter::create(inst, memory->getParameter(NLName("RD_PORTS")), "3");
  SNLInstParameter::create(inst, memory->getParameter(NLName("WR_PORTS")), "2");
  SNLInstParameter::create(inst, memory->getParameter(NLName("RST_ENABLE")), "1");
  SNLInstParameter::create(inst, memory->getParameter(NLName("RST_ASYNC")), "0");
  SNLInstParameter::create(inst, memory->getParameter(NLName("RST_ACTIVE_LOW")), "1");

  const auto signature = NLDB0::getMemorySignature(inst);
  EXPECT_EQ(7u, signature.width);
  EXPECT_EQ(4u, signature.depth);
  EXPECT_EQ(2u, signature.abits);
  EXPECT_EQ(3u, signature.readPorts);
  EXPECT_EQ(2u, signature.writePorts);
  EXPECT_EQ(NLDB0::MemoryResetMode::SyncLow, signature.resetMode);
}

TEST_F(NLDB0Test, testMemoryPrimitiveSyncResetModes) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto checkMemory = [](NLDB0::MemoryResetMode mode,
                        const char* expectedName,
                        const char* expectedAsync,
                        const char* expectedActiveLow) {
    NLDB0::MemorySignature signature;
    signature.width = 4;
    signature.depth = 8;
    signature.abits = 3;
    signature.readPorts = 1;
    signature.writePorts = 1;
    signature.resetMode = mode;

    auto* memory = NLDB0::getOrCreateMemory(signature);
    ASSERT_NE(nullptr, memory);
    EXPECT_EQ(NLName(expectedName), memory->getName());
    EXPECT_EQ("1", memory->getParameter(NLName("RST_ENABLE"))->getValue());
    EXPECT_EQ(expectedAsync, memory->getParameter(NLName("RST_ASYNC"))->getValue());
    EXPECT_EQ(expectedActiveLow, memory->getParameter(NLName("RST_ACTIVE_LOW"))->getValue());
  };

  checkMemory(
    NLDB0::MemoryResetMode::SyncLow,
    "naja_mem__w4_d8_a3_r1_w1_rst_sync_low",
    "0",
    "1");
  checkMemory(
    NLDB0::MemoryResetMode::SyncHigh,
    "naja_mem__w4_d8_a3_r1_w1_rst_sync_high",
    "0",
    "0");
}

TEST_F(NLDB0Test, testMemoryRecognitionByStableSchemaOutsideDb0Library) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto* db = NLDB::create(NLUniverse::get());
  ASSERT_NE(nullptr, db);
  auto* library =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("work"));
  ASSERT_NE(nullptr, library);

  auto* memory = SNLDesign::create(
      library,
      SNLDesign::Type::Primitive,
      NLName("naja_mem__w8_d4_a2_r1_w1_rst_async_low"));
  ASSERT_NE(nullptr, memory);
  EXPECT_FALSE(NLDB0::isDB0Primitive(memory));

  SNLParameter::create(memory, NLName("WIDTH"), SNLParameter::Type::Decimal, "8");
  SNLParameter::create(memory, NLName("DEPTH"), SNLParameter::Type::Decimal, "4");
  SNLParameter::create(memory, NLName("ABITS"), SNLParameter::Type::Decimal, "2");
  SNLParameter::create(memory, NLName("RD_PORTS"), SNLParameter::Type::Decimal, "1");
  SNLParameter::create(memory, NLName("WR_PORTS"), SNLParameter::Type::Decimal, "1");
  SNLParameter::create(memory, NLName("RST_ENABLE"), SNLParameter::Type::Decimal, "1");
  SNLParameter::create(memory, NLName("RST_ASYNC"), SNLParameter::Type::Decimal, "1");
  SNLParameter::create(memory, NLName("RST_ACTIVE_LOW"), SNLParameter::Type::Decimal, "1");
  SNLParameter::create(memory, NLName("INIT"), SNLParameter::Type::Binary, "32'b0");

  auto* clk = SNLScalarTerm::create(memory, SNLTerm::Direction::Input, NLName("CLK"));
  auto* rst = SNLScalarTerm::create(memory, SNLTerm::Direction::Input, NLName("RST"));
  auto* raddr =
      SNLBusTerm::create(memory, SNLTerm::Direction::Input, 1, 0, NLName("RADDR"));
  auto* rdata =
      SNLBusTerm::create(memory, SNLTerm::Direction::Output, 7, 0, NLName("RDATA"));
  auto* waddr =
      SNLBusTerm::create(memory, SNLTerm::Direction::Input, 1, 0, NLName("WADDR"));
  auto* wdata =
      SNLBusTerm::create(memory, SNLTerm::Direction::Input, 7, 0, NLName("WDATA"));
  auto* we = SNLBusTerm::create(memory, SNLTerm::Direction::Input, 0, 0, NLName("WE"));

  EXPECT_TRUE(NLDB0::isMemory(memory));
  const auto signature = NLDB0::getMemorySignature(memory);
  EXPECT_EQ(8u, signature.width);
  EXPECT_EQ(4u, signature.depth);
  EXPECT_EQ(2u, signature.abits);
  EXPECT_EQ(1u, signature.readPorts);
  EXPECT_EQ(1u, signature.writePorts);
  EXPECT_EQ(NLDB0::MemoryResetMode::AsyncLow, signature.resetMode);
  EXPECT_EQ(clk, NLDB0::getMemoryClock(memory));
  EXPECT_EQ(rst, NLDB0::getMemoryReset(memory));
  EXPECT_EQ(raddr, NLDB0::getMemoryReadAddress(memory));
  EXPECT_EQ(rdata, NLDB0::getMemoryReadData(memory));
  EXPECT_EQ(waddr, NLDB0::getMemoryWriteAddress(memory));
  EXPECT_EQ(wdata, NLDB0::getMemoryWriteData(memory));
  EXPECT_EQ(we, NLDB0::getMemoryWriteEnable(memory));
}

TEST_F(NLDB0Test, testSequentialPrimitiveModeling) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto* db = NLDB::create(NLUniverse::get());
  ASSERT_NE(nullptr, db);
  auto* library = NLLibrary::create(db, NLName("designs"));
  ASSERT_NE(nullptr, library);
  auto* top = SNLDesign::create(library, NLName("top"));
  ASSERT_NE(nullptr, top);

  const std::vector<SequentialPrimitiveData> sequentialPrimitives = {
    {
      NLDB0::getDLatch(),
      NLDB0::getDLatchEnable(),
      {NLDB0::getDLatchData()},
      {NLDB0::getDLatchOutput()},
    },
    {
      NLDB0::getDFF(),
      NLDB0::getDFFClock(),
      {NLDB0::getDFFData()},
      {NLDB0::getDFFOutput()},
    },
    {
      NLDB0::getDFFN(),
      NLDB0::getDFFNClock(),
      {NLDB0::getDFFNData()},
      {NLDB0::getDFFNOutput()},
    },
    {
      NLDB0::getDFFRN(),
      NLDB0::getDFFRNClock(),
      {NLDB0::getDFFRNData(), NLDB0::getDFFRNResetN()},
      {NLDB0::getDFFRNOutput()},
    },
    {
      NLDB0::getDFFE(),
      NLDB0::getDFFEClock(),
      {NLDB0::getDFFEData(), NLDB0::getDFFEEnable()},
      {NLDB0::getDFFEOutput()},
    },
    {
      NLDB0::getDFFRE(),
      NLDB0::getDFFREClock(),
      {NLDB0::getDFFREData(), NLDB0::getDFFREEnable(), NLDB0::getDFFREReset()},
      {NLDB0::getDFFREOutput()},
    },
    {
      NLDB0::getDFFSE(),
      NLDB0::getDFFSEClock(),
      {NLDB0::getDFFSEData(), NLDB0::getDFFSEEnable(), NLDB0::getDFFSESet()},
      {NLDB0::getDFFSEOutput()},
    },
  };

  for (const auto& primitive: sequentialPrimitives) {
    ASSERT_NE(nullptr, primitive.design);
    ASSERT_NE(nullptr, primitive.clock);
    EXPECT_TRUE(NLDB0::isDB0Primitive(primitive.design));
    EXPECT_TRUE(SNLDesignModeling::isSequential(primitive.design));
    EXPECT_EQ(0u, SNLDesignModeling::getTruthTableCount(primitive.design));
    EXPECT_FALSE(SNLDesignModeling::getTruthTable(primitive.design).isInitialized());

    for (auto* term: primitive.design->getBitTerms()) {
      EXPECT_FALSE(SNLDesignModeling::areDependenciesDefined(term));
      EXPECT_FALSE(
        SNLDesignModeling::getTruthTable(primitive.design, term->getFlatID()).isInitialized());
    }

    expectTerms(
      collectTerms(SNLDesignModeling::getClockRelatedInputs(primitive.clock)),
      primitive.inputs);
    expectTerms(
      collectTerms(SNLDesignModeling::getClockRelatedOutputs(primitive.clock)),
      primitive.outputs);
    EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(primitive.clock).empty());
    EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(primitive.clock).empty());

    for (auto* input: primitive.inputs) {
      expectTerms(
        collectTerms(SNLDesignModeling::getInputRelatedClocks(input)),
        {primitive.clock});
      EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(input).empty());
      EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(input).empty());
      EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(input).empty());
    }
    for (auto* output: primitive.outputs) {
      expectTerms(
        collectTerms(SNLDesignModeling::getOutputRelatedClocks(output)),
        {primitive.clock});
      EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(output).empty());
      EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(output).empty());
      EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(output).empty());
    }

    auto* instance = SNLInstance::create(
      top,
      primitive.design,
      NLName(primitive.design->getName().getString() + "_inst"));
    ASSERT_NE(nullptr, instance);

    std::vector<SNLInstTerm*> inputInstTerms;
    inputInstTerms.reserve(primitive.inputs.size());
    for (auto* input: primitive.inputs) {
      inputInstTerms.push_back(instance->getInstTerm(input));
    }
    std::vector<SNLInstTerm*> outputInstTerms;
    outputInstTerms.reserve(primitive.outputs.size());
    for (auto* output: primitive.outputs) {
      outputInstTerms.push_back(instance->getInstTerm(output));
    }
    auto* clockInstTerm = instance->getInstTerm(primitive.clock);
    ASSERT_NE(nullptr, clockInstTerm);

    expectTerms(
      collectTerms(SNLDesignModeling::getClockRelatedInputs(clockInstTerm)),
      inputInstTerms);
    expectTerms(
      collectTerms(SNLDesignModeling::getClockRelatedOutputs(clockInstTerm)),
      outputInstTerms);
    EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(clockInstTerm).empty());
    EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(clockInstTerm).empty());

    for (auto* input: inputInstTerms) {
      expectTerms(
        collectTerms(SNLDesignModeling::getInputRelatedClocks(input)),
        {clockInstTerm});
      EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(input).empty());
      EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(input).empty());
      EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(input).empty());
    }
    for (auto* output: outputInstTerms) {
      expectTerms(
        collectTerms(SNLDesignModeling::getOutputRelatedClocks(output)),
        {clockInstTerm});
      EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(output).empty());
      EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(output).empty());
      EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(output).empty());
    }
  }
}

TEST_F(NLDB0Test, testDFFRN) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto dffrn = NLDB0::getDFFRN();
  ASSERT_NE(nullptr, dffrn);
  EXPECT_TRUE(NLDB0::isDFFRN(dffrn));
  EXPECT_TRUE(NLDB0::isDB0Primitive(dffrn));
  EXPECT_EQ(NLName("naja_dffrn"), dffrn->getName());
  EXPECT_FALSE(NLDB0::isDFFRN(NLDB0::getDFF()));

  auto clock = NLDB0::getDFFRNClock();
  auto data = NLDB0::getDFFRNData();
  auto resetN = NLDB0::getDFFRNResetN();
  auto output = NLDB0::getDFFRNOutput();
  ASSERT_NE(nullptr, clock);
  ASSERT_NE(nullptr, data);
  ASSERT_NE(nullptr, resetN);
  ASSERT_NE(nullptr, output);
  EXPECT_EQ(NLName("C"), clock->getName());
  EXPECT_EQ(NLName("D"), data->getName());
  EXPECT_EQ(NLName("RN"), resetN->getName());
  EXPECT_EQ(NLName("Q"), output->getName());
  EXPECT_EQ(SNLTerm::Direction::Input, clock->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, data->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, resetN->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, output->getDirection());
}

TEST_F(NLDB0Test, testDFFN) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());
  auto rootLibrary = NLDB0::getDB0RootLibrary();
  ASSERT_NE(nullptr, rootLibrary);
  EXPECT_EQ(nullptr, rootLibrary->getSNLDesign(NLName("naja_dffn")));

  auto dffn = NLDB0::getDFFN();
  ASSERT_NE(nullptr, dffn);
  EXPECT_TRUE(NLDB0::isDFFN(dffn));
  EXPECT_TRUE(NLDB0::isDB0Primitive(dffn));
  EXPECT_EQ(NLName("naja_dffn"), dffn->getName());
  EXPECT_FALSE(NLDB0::isDFFN(NLDB0::getDFF()));

  auto clock = NLDB0::getDFFNClock();
  auto data = NLDB0::getDFFNData();
  auto output = NLDB0::getDFFNOutput();
  ASSERT_NE(nullptr, clock);
  ASSERT_NE(nullptr, data);
  ASSERT_NE(nullptr, output);
  EXPECT_EQ(NLName("C"), clock->getName());
  EXPECT_EQ(NLName("D"), data->getName());
  EXPECT_EQ(NLName("Q"), output->getName());
  EXPECT_EQ(SNLTerm::Direction::Input, clock->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, data->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, output->getDirection());
}

TEST_F(NLDB0Test, testDLatch) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());
  auto rootLibrary = NLDB0::getDB0RootLibrary();
  ASSERT_NE(nullptr, rootLibrary);
  EXPECT_EQ(nullptr, rootLibrary->getSNLDesign(NLName("naja_dlatch")));

  auto dlatch = NLDB0::getDLatch();
  ASSERT_NE(nullptr, dlatch);
  EXPECT_TRUE(NLDB0::isDLatch(dlatch));
  EXPECT_TRUE(NLDB0::isDB0Primitive(dlatch));
  EXPECT_EQ(NLName("naja_dlatch"), dlatch->getName());
  EXPECT_FALSE(NLDB0::isDLatch(NLDB0::getDFF()));

  auto enable = NLDB0::getDLatchEnable();
  auto data = NLDB0::getDLatchData();
  auto output = NLDB0::getDLatchOutput();
  ASSERT_NE(nullptr, enable);
  ASSERT_NE(nullptr, data);
  ASSERT_NE(nullptr, output);
  EXPECT_EQ(NLName("E"), enable->getName());
  EXPECT_EQ(NLName("D"), data->getName());
  EXPECT_EQ(NLName("Q"), output->getName());
  EXPECT_EQ(SNLTerm::Direction::Input, enable->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, data->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, output->getDirection());
}

TEST_F(NLDB0Test, testDFFE_DFFRE_DFFSE) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());
  auto rootLibrary = NLDB0::getDB0RootLibrary();
  ASSERT_NE(nullptr, rootLibrary);
  EXPECT_EQ(nullptr, rootLibrary->getSNLDesign(NLName("naja_dffe")));
  EXPECT_EQ(nullptr, rootLibrary->getSNLDesign(NLName("naja_dffre")));
  EXPECT_EQ(nullptr, rootLibrary->getSNLDesign(NLName("naja_dffse")));

  auto dffe = NLDB0::getDFFE();
  ASSERT_NE(nullptr, dffe);
  EXPECT_TRUE(NLDB0::isDFFE(dffe));
  EXPECT_EQ(NLName("naja_dffe"), dffe->getName());
  ASSERT_NE(nullptr, NLDB0::getDFFEClock());
  ASSERT_NE(nullptr, NLDB0::getDFFEData());
  ASSERT_NE(nullptr, NLDB0::getDFFEEnable());
  ASSERT_NE(nullptr, NLDB0::getDFFEOutput());
  EXPECT_EQ(NLName("C"), NLDB0::getDFFEClock()->getName());
  EXPECT_EQ(NLName("D"), NLDB0::getDFFEData()->getName());
  EXPECT_EQ(NLName("E"), NLDB0::getDFFEEnable()->getName());
  EXPECT_EQ(NLName("Q"), NLDB0::getDFFEOutput()->getName());

  auto dffre = NLDB0::getDFFRE();
  ASSERT_NE(nullptr, dffre);
  EXPECT_TRUE(NLDB0::isDFFRE(dffre));
  EXPECT_EQ(NLName("naja_dffre"), dffre->getName());
  ASSERT_NE(nullptr, NLDB0::getDFFREClock());
  ASSERT_NE(nullptr, NLDB0::getDFFREData());
  ASSERT_NE(nullptr, NLDB0::getDFFREEnable());
  ASSERT_NE(nullptr, NLDB0::getDFFREReset());
  ASSERT_NE(nullptr, NLDB0::getDFFREOutput());
  EXPECT_EQ(NLName("C"), NLDB0::getDFFREClock()->getName());
  EXPECT_EQ(NLName("D"), NLDB0::getDFFREData()->getName());
  EXPECT_EQ(NLName("E"), NLDB0::getDFFREEnable()->getName());
  EXPECT_EQ(NLName("R"), NLDB0::getDFFREReset()->getName());
  EXPECT_EQ(NLName("Q"), NLDB0::getDFFREOutput()->getName());

  auto dffse = NLDB0::getDFFSE();
  ASSERT_NE(nullptr, dffse);
  EXPECT_TRUE(NLDB0::isDFFSE(dffse));
  EXPECT_EQ(NLName("naja_dffse"), dffse->getName());
  ASSERT_NE(nullptr, NLDB0::getDFFSEClock());
  ASSERT_NE(nullptr, NLDB0::getDFFSEData());
  ASSERT_NE(nullptr, NLDB0::getDFFSEEnable());
  ASSERT_NE(nullptr, NLDB0::getDFFSESet());
  ASSERT_NE(nullptr, NLDB0::getDFFSEOutput());
  EXPECT_EQ(NLName("C"), NLDB0::getDFFSEClock()->getName());
  EXPECT_EQ(NLName("D"), NLDB0::getDFFSEData()->getName());
  EXPECT_EQ(NLName("E"), NLDB0::getDFFSEEnable()->getName());
  EXPECT_EQ(NLName("S"), NLDB0::getDFFSESet()->getName());
  EXPECT_EQ(NLName("Q"), NLDB0::getDFFSEOutput()->getName());
}

TEST_F(NLDB0Test, testNULLUniverse) {
  EXPECT_EQ(nullptr, NLUniverse::get());
  EXPECT_FALSE(NLUniverse::isDB0(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getAssign());
  EXPECT_EQ(nullptr, NLDB0::getAssignInput());
  EXPECT_EQ(nullptr, NLDB0::getAssignOutput());
  EXPECT_EQ(nullptr, NLDB0::getFA());
  EXPECT_EQ(nullptr, NLDB0::getFAInputA());
  EXPECT_EQ(nullptr, NLDB0::getFAInputB());
  EXPECT_EQ(nullptr, NLDB0::getFAInputCI());
  EXPECT_EQ(nullptr, NLDB0::getFAOutputS());
  EXPECT_EQ(nullptr, NLDB0::getFAOutputCO());
  EXPECT_EQ(nullptr, NLDB0::getMux2());
  EXPECT_EQ(nullptr, NLDB0::getOrCreateMux2(2));
  EXPECT_FALSE(NLDB0::isMux2(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getMux2InputA());
  EXPECT_EQ(nullptr, NLDB0::getMux2InputB());
  EXPECT_EQ(nullptr, NLDB0::getMux2Select());
  EXPECT_EQ(nullptr, NLDB0::getMux2Output());
  EXPECT_EQ(nullptr, NLDB0::getDFF());
  EXPECT_EQ(nullptr, NLDB0::getDFFClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFData());
  EXPECT_EQ(nullptr, NLDB0::getDFFOutput());
  EXPECT_EQ(nullptr, NLDB0::getDLatch());
  EXPECT_FALSE(NLDB0::isDLatch(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getDLatchEnable());
  EXPECT_EQ(nullptr, NLDB0::getDLatchData());
  EXPECT_EQ(nullptr, NLDB0::getDLatchOutput());
  EXPECT_EQ(nullptr, NLDB0::getDFFN());
  EXPECT_FALSE(NLDB0::isDFFN(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getDFFNClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFNData());
  EXPECT_EQ(nullptr, NLDB0::getDFFNOutput());
  EXPECT_EQ(nullptr, NLDB0::getDFFRN());
  EXPECT_FALSE(NLDB0::isDFFRN(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getDFFRNClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFRNData());
  EXPECT_EQ(nullptr, NLDB0::getDFFRNResetN());
  EXPECT_EQ(nullptr, NLDB0::getDFFRNOutput());
  EXPECT_EQ(nullptr, NLDB0::getDFFE());
  EXPECT_FALSE(NLDB0::isDFFE(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getDFFEClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFEData());
  EXPECT_EQ(nullptr, NLDB0::getDFFEEnable());
  EXPECT_EQ(nullptr, NLDB0::getDFFEOutput());
  EXPECT_EQ(nullptr, NLDB0::getDFFRE());
  EXPECT_FALSE(NLDB0::isDFFRE(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getDFFREClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFREData());
  EXPECT_EQ(nullptr, NLDB0::getDFFREEnable());
  EXPECT_EQ(nullptr, NLDB0::getDFFREReset());
  EXPECT_EQ(nullptr, NLDB0::getDFFREOutput());
  EXPECT_EQ(nullptr, NLDB0::getDFFSE());
  EXPECT_FALSE(NLDB0::isDFFSE(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getDFFSEClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFSEData());
  EXPECT_EQ(nullptr, NLDB0::getDFFSEEnable());
  EXPECT_EQ(nullptr, NLDB0::getDFFSESet());
  EXPECT_EQ(nullptr, NLDB0::getDFFSEOutput());
  EXPECT_EQ(nullptr, NLDB0::getGateLibrary(NLDB0::GateType::And));
  NLDB0::MemorySignature signature;
  signature.width = 4;
  signature.depth = 8;
  signature.abits = 3;
  signature.readPorts = 1;
  signature.writePorts = 1;
  EXPECT_EQ(nullptr, NLDB0::getOrCreateMemory(signature));
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2), NLException);
}

TEST_F(NLDB0Test, testErrors) {
  EXPECT_FALSE(NLDB0::isNInputGate(nullptr));
  EXPECT_FALSE(NLDB0::isNOutputGate(nullptr));
  EXPECT_FALSE(NLDB0::isGate(nullptr));

  auto universe = NLUniverse::create();
  ASSERT_NE(nullptr, universe);
  auto db = NLDB::create(universe);
  ASSERT_NE(nullptr, db);
  auto library = NLLibrary::create(db, NLName("testLibrary"));

  auto childLibrary = NLLibrary::create(library, NLName("childLibrary"));
  ASSERT_NE(nullptr, childLibrary);
  EXPECT_FALSE(NLDB0::isGateLibrary(childLibrary));

  auto model = SNLDesign::create(library, NLName("testGate"));
  EXPECT_FALSE(NLDB0::isNInputGate(model));
  EXPECT_FALSE(NLDB0::isNOutputGate(model));
  EXPECT_FALSE(NLDB0::isGate(model));
  EXPECT_EQ(std::string(), NLDB0::getGateName(model));
  EXPECT_EQ(nullptr, NLDB0::getGateSingleTerm(model));
  EXPECT_EQ(nullptr, NLDB0::getGateNTerms(model));
  NLDB0::MemorySignature invalidMemorySignature;
  invalidMemorySignature.depth = 8;
  invalidMemorySignature.abits = 3;
  invalidMemorySignature.readPorts = 1;
  invalidMemorySignature.writePorts = 1;
  EXPECT_THROW(NLDB0::getOrCreateMemory(invalidMemorySignature), NLException);
  EXPECT_THROW(NLDB0::getOrCreateMux2(0), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 0), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 0), NLException);
  NLUniverse::get()->destroy();
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2), NLException);
}
