// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "NLException.h"
using namespace naja::NL;

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

TEST_F(NLDB0Test, testMux2TruthTable) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto mux2 = NLDB0::getMux2();
  ASSERT_NE(nullptr, mux2);
  EXPECT_TRUE(NLDB0::isMux2(mux2));
  EXPECT_EQ(NLName("naja_mux2"), mux2->getName());
  ASSERT_NE(nullptr, NLDB0::getMux2InputA());
  ASSERT_NE(nullptr, NLDB0::getMux2InputB());
  ASSERT_NE(nullptr, NLDB0::getMux2Select());
  ASSERT_NE(nullptr, NLDB0::getMux2Output());

  auto tt = NLDB0::getPrimitiveTruthTable(mux2);
  EXPECT_EQ(3u, tt.size());

  uint64_t bits = 0;
  for (uint64_t i = 0; i < (1ULL << tt.size()); ++i) {
    if (tt.bits().bit(i)) {
      bits |= (1ULL << i);
    }
  }
  // Truth table for Y = S ? B : A, with A/B/S mapped to input bits 0/1/2.
  EXPECT_EQ(0xCAULL, bits);
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

  auto* memory1 = NLDB0::getOrCreateMemory(signature);
  EXPECT_EQ(memory0, memory1);

  ASSERT_NE(nullptr, memory0->getScalarTerm(NLName("CLK")));
  ASSERT_NE(nullptr, memory0->getScalarTerm(NLName("RST")));
  auto* raddr = memory0->getBusTerm(NLName("RADDR"));
  auto* rdata = memory0->getBusTerm(NLName("RDATA"));
  auto* waddr = memory0->getBusTerm(NLName("WADDR"));
  auto* wdata = memory0->getBusTerm(NLName("WDATA"));
  auto* we = memory0->getBusTerm(NLName("WE"));
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

TEST_F(NLDB0Test, testDFFE_DFFRE_DFFSE) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

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
  EXPECT_FALSE(NLDB0::isMux2(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getMux2InputA());
  EXPECT_EQ(nullptr, NLDB0::getMux2InputB());
  EXPECT_EQ(nullptr, NLDB0::getMux2Select());
  EXPECT_EQ(nullptr, NLDB0::getMux2Output());
  EXPECT_EQ(nullptr, NLDB0::getDFF());
  EXPECT_EQ(nullptr, NLDB0::getDFFClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFData());
  EXPECT_EQ(nullptr, NLDB0::getDFFOutput());
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
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 0), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 0), NLException);
  NLUniverse::get()->destroy();
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2), NLException);
}
