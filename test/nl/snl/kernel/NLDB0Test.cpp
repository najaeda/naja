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
#include "SNLInstance.h"
#include "SNLParameter.h"
#include "NLException.h"
#include <algorithm>
#include <limits>
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

NLID::LibraryID expectedSequentialLibraryID(const NLName& name) {
  if (name == NLName("naja_dff")) {
    return NLID::LibraryID(3);
  }
  if (name == NLName("naja_dffn")) {
    return NLID::LibraryID(4);
  }
  if (name == NLName("naja_dffrn")) {
    return NLID::LibraryID(5);
  }
  if (name == NLName("naja_dffr")) {
    return NLID::LibraryID(6);
  }
  if (name == NLName("naja_dffs")) {
    return NLID::LibraryID(7);
  }
  if (name == NLName("naja_dffe")) {
    return NLID::LibraryID(8);
  }
  if (name == NLName("naja_dffre")) {
    return NLID::LibraryID(9);
  }
  if (name == NLName("naja_dffse")) {
    return NLID::LibraryID(10);
  }
  if (name == NLName("naja_dlatch")) {
    return NLID::LibraryID(11);
  }
  if (name == NLName("naja_dffsr")) {
    return NLID::LibraryID(17);
  }
  if (name == NLName("naja_dffsrn")) {
    return NLID::LibraryID(18);
  }
  if (name == NLName("naja_dffss")) {
    return NLID::LibraryID(19);
  }
  if (name == NLName("naja_dffssn")) {
    return NLID::LibraryID(20);
  }
  if (name == NLName("naja_dffsre")) {
    return NLID::LibraryID(21);
  }
  if (name == NLName("naja_dffsrne")) {
    return NLID::LibraryID(22);
  }
  if (name == NLName("naja_dffsse")) {
    return NLID::LibraryID(23);
  }
  if (name == NLName("naja_dffssne")) {
    return NLID::LibraryID(24);
  }
  return NLID::LibraryID(0);
}

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
  EXPECT_EQ(1, assign->getID());
  ASSERT_NE(nullptr, assign->getLibrary());
  EXPECT_EQ(1, assign->getLibrary()->getID());
  EXPECT_EQ(NLName("assign"), assign->getLibrary()->getName());
  EXPECT_EQ(NLDB0::getDB0RootLibrary(), assign->getLibrary()->getParentLibrary());
  EXPECT_EQ(assign, assign->getLibrary()->getSNLDesign(NLID::DesignID(1)));
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

TEST_F(NLDB0Test, testDFFInitValueFormatting) {
  EXPECT_EQ("4'b01xz", NLDB0::formatDFFInitValue(4, "01XZ"));
  EXPECT_EQ("3'bxxx", NLDB0::getUndefinedDFFInitValue(3));

  EXPECT_THROW(NLDB0::formatDFFInitValue(0, ""), NLException);
  EXPECT_THROW(NLDB0::formatDFFInitValue(2, "0"), NLException);
  EXPECT_THROW(NLDB0::formatDFFInitValue(1, "2"), NLException);
}

TEST_F(NLDB0Test, testPrimitiveTermRoles) {
  using Role = SNLDesignModeling::SNLTermRole;
  using Level = SNLDesignModeling::SNLActiveLevel;
  NLUniverse::create();

  auto checkBasic = [](SNLDesign* design, SNLBitTerm* clock,
                       SNLBitTerm* data, SNLBitTerm* output) {
    EXPECT_EQ(Role::Clock, SNLDesignModeling::getTermRole(clock));
    EXPECT_EQ(Role::DataInput, SNLDesignModeling::getTermRole(data));
    EXPECT_EQ(Role::DataOutput, SNLDesignModeling::getTermRole(output));
    EXPECT_EQ(1, SNLDesignModeling::getClockTerms(design).size());
    EXPECT_EQ(1, SNLDesignModeling::getDataInputTerms(design).size());
    EXPECT_EQ(1, SNLDesignModeling::getOutputTerms(design).size());
  };

  checkBasic(NLDB0::getDFF(), NLDB0::getDFFClock(),
             NLDB0::getDFFData(), NLDB0::getDFFOutput());
  checkBasic(NLDB0::getDFFN(), NLDB0::getDFFNClock(),
             NLDB0::getDFFNData(), NLDB0::getDFFNOutput());
  checkBasic(NLDB0::getDFFRN(), NLDB0::getDFFRNClock(),
             NLDB0::getDFFRNData(), NLDB0::getDFFRNOutput());
  checkBasic(NLDB0::getDFFR(), NLDB0::getDFFRClock(),
             NLDB0::getDFFRData(), NLDB0::getDFFROutput());
  checkBasic(NLDB0::getDFFS(), NLDB0::getDFFSClock(),
             NLDB0::getDFFSData(), NLDB0::getDFFSOutput());
  checkBasic(NLDB0::getDFFE(), NLDB0::getDFFEClock(),
             NLDB0::getDFFEData(), NLDB0::getDFFEOutput());
  checkBasic(NLDB0::getDFFRE(), NLDB0::getDFFREClock(),
             NLDB0::getDFFREData(), NLDB0::getDFFREOutput());
  checkBasic(NLDB0::getDFFSE(), NLDB0::getDFFSEClock(),
             NLDB0::getDFFSEData(), NLDB0::getDFFSEOutput());
  checkBasic(NLDB0::getDFFSR(), NLDB0::getDFFSRClock(),
             NLDB0::getDFFSRData(), NLDB0::getDFFSROutput());
  checkBasic(NLDB0::getDFFSRN(), NLDB0::getDFFSRNClock(),
             NLDB0::getDFFSRNData(), NLDB0::getDFFSRNOutput());
  checkBasic(NLDB0::getDFFSS(), NLDB0::getDFFSSClock(),
             NLDB0::getDFFSSData(), NLDB0::getDFFSSOutput());
  checkBasic(NLDB0::getDFFSSN(), NLDB0::getDFFSSNClock(),
             NLDB0::getDFFSSNData(), NLDB0::getDFFSSNOutput());

  EXPECT_EQ(Role::AsyncReset,
            SNLDesignModeling::getTermRole(NLDB0::getDFFRNResetN()));
  EXPECT_EQ(Level::Low,
            SNLDesignModeling::getResetActiveLevel(NLDB0::getDFFRNResetN()));
  EXPECT_EQ(Role::AsyncReset,
            SNLDesignModeling::getTermRole(NLDB0::getDFFRReset()));
  EXPECT_EQ(Level::High,
            SNLDesignModeling::getResetActiveLevel(NLDB0::getDFFRReset()));
  EXPECT_EQ(Role::AsyncSet,
            SNLDesignModeling::getTermRole(NLDB0::getDFFSSet()));
  EXPECT_EQ(Level::High,
            SNLDesignModeling::getResetActiveLevel(NLDB0::getDFFSSet()));
  EXPECT_EQ(Role::Enable,
            SNLDesignModeling::getTermRole(NLDB0::getDFFEEnable()));
  EXPECT_EQ(Role::Enable,
            SNLDesignModeling::getTermRole(NLDB0::getDFFREEnable()));
  EXPECT_EQ(Role::AsyncReset,
            SNLDesignModeling::getTermRole(NLDB0::getDFFREReset()));
  EXPECT_EQ(Role::Enable,
            SNLDesignModeling::getTermRole(NLDB0::getDFFSEEnable()));
  EXPECT_EQ(Role::AsyncSet,
            SNLDesignModeling::getTermRole(NLDB0::getDFFSESet()));
  EXPECT_EQ(Role::SyncReset,
            SNLDesignModeling::getTermRole(NLDB0::getDFFSRReset()));
  EXPECT_EQ(Level::High,
            SNLDesignModeling::getResetActiveLevel(NLDB0::getDFFSRReset()));
  EXPECT_EQ(Role::SyncReset,
            SNLDesignModeling::getTermRole(NLDB0::getDFFSRNResetN()));
  EXPECT_EQ(Level::Low,
            SNLDesignModeling::getResetActiveLevel(NLDB0::getDFFSRNResetN()));
  EXPECT_EQ(Role::SyncSet,
            SNLDesignModeling::getTermRole(NLDB0::getDFFSSSet()));
  EXPECT_EQ(Level::High,
            SNLDesignModeling::getResetActiveLevel(NLDB0::getDFFSSSet()));
  EXPECT_EQ(Role::SyncSet,
            SNLDesignModeling::getTermRole(NLDB0::getDFFSSNSetN()));
  EXPECT_EQ(Level::Low,
            SNLDesignModeling::getResetActiveLevel(NLDB0::getDFFSSNSetN()));
  EXPECT_TRUE(SNLDesignModeling::getAsyncResetTerms(NLDB0::getDFF()).empty());
  EXPECT_EQ(1, SNLDesignModeling::getSyncResetTerms(NLDB0::getDFFSR()).size());
  EXPECT_EQ(1, SNLDesignModeling::getSyncSetTerms(NLDB0::getDFFSS()).size());

  NLDB0::MemorySignature signature;
  signature.width = 4;
  signature.depth = 8;
  signature.abits = 3;
  signature.readPorts = 1;
  signature.writePorts = 1;
  signature.resetMode = NLDB0::MemoryResetMode::AsyncLow;
  auto* memory = NLDB0::getOrCreateMemory(signature);
  EXPECT_EQ(Role::Clock,
            SNLDesignModeling::getTermRole(NLDB0::getMemoryClock(memory)));
  EXPECT_EQ(Role::AsyncReset,
            SNLDesignModeling::getTermRole(NLDB0::getMemoryReset(memory)));
  EXPECT_EQ(Level::Low,
            SNLDesignModeling::getResetActiveLevel(NLDB0::getMemoryReset(memory)));
  EXPECT_EQ(Role::MemoryReadAddress, SNLDesignModeling::getTermRole(
      NLDB0::getMemoryReadAddress(memory)->getBit(0)));
  EXPECT_EQ(Role::MemoryReadData, SNLDesignModeling::getTermRole(
      NLDB0::getMemoryReadData(memory)->getBit(0)));
  EXPECT_EQ(Role::MemoryWriteAddress, SNLDesignModeling::getTermRole(
      NLDB0::getMemoryWriteAddress(memory)->getBit(0)));
  EXPECT_EQ(Role::MemoryWriteData, SNLDesignModeling::getTermRole(
      NLDB0::getMemoryWriteData(memory)->getBit(0)));
  EXPECT_EQ(Role::MemoryWriteEnable, SNLDesignModeling::getTermRole(
      NLDB0::getMemoryWriteEnable(memory)->getBit(0)));
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

TEST_F(NLDB0Test, testGatePrimitiveModeling) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto* db = NLDB::create(NLUniverse::get());
  ASSERT_NE(nullptr, db);
  auto* library = NLLibrary::create(db, NLName("WORK"));
  ASSERT_NE(nullptr, library);
  auto* top = SNLDesign::create(library, NLName("top"));
  ASSERT_NE(nullptr, top);

  const std::vector<NLDB0::GateType> nInputTypes = {
    NLDB0::GateType::And,
    NLDB0::GateType::Nand,
    NLDB0::GateType::Or,
    NLDB0::GateType::Nor,
    NLDB0::GateType::Xor,
    NLDB0::GateType::Xnor,
  };
  for (const auto& type: nInputTypes) {
    auto* gate = NLDB0::getOrCreateNInputGate(type, 4);
    ASSERT_NE(nullptr, gate);
    EXPECT_TRUE(SNLDesignModeling::hasModeling(gate));

    auto* output = NLDB0::getGateSingleTerm(gate);
    auto* inputs = NLDB0::getGateNTerms(gate);
    ASSERT_NE(nullptr, output);
    ASSERT_NE(nullptr, inputs);

    std::vector<SNLBitTerm*> expectedInputs;
    for (auto* input: inputs->getBits()) {
      expectedInputs.push_back(input);
      expectTerms(
        collectTerms(SNLDesignModeling::getCombinatorialOutputs(input)),
        std::vector<SNLBitTerm*>({output}));
    }
    expectTerms(
      collectTerms(SNLDesignModeling::getCombinatorialInputs(output)),
      expectedInputs);

    auto* instance =
      SNLInstance::create(top, gate, NLName(type.getString() + "_gate"));
    ASSERT_NE(nullptr, instance);
    std::vector<SNLInstTerm*> expectedInstInputs;
    for (auto* input: inputs->getBits()) {
      expectedInstInputs.push_back(instance->getInstTerm(input));
    }
    expectTerms(
      collectTerms(
        SNLDesignModeling::getCombinatorialInputs(instance->getInstTerm(output))),
      expectedInstInputs);

    EXPECT_EQ(gate, NLDB0::getOrCreateNInputGate(type, 4));
    EXPECT_EQ(
      expectedInputs.size(),
      SNLDesignModeling::getCombinatorialInputs(output).size());
  }

  const std::vector<NLDB0::GateType> nOutputTypes = {
    NLDB0::GateType::Buf,
    NLDB0::GateType::Not,
  };
  for (const auto& type: nOutputTypes) {
    auto* gate = NLDB0::getOrCreateNOutputGate(type, 4);
    ASSERT_NE(nullptr, gate);
    EXPECT_TRUE(SNLDesignModeling::hasModeling(gate));

    auto* input = NLDB0::getGateSingleTerm(gate);
    auto* outputs = NLDB0::getGateNTerms(gate);
    ASSERT_NE(nullptr, input);
    ASSERT_NE(nullptr, outputs);

    std::vector<SNLBitTerm*> expectedOutputs;
    for (auto* output: outputs->getBits()) {
      expectedOutputs.push_back(output);
      expectTerms(
        collectTerms(SNLDesignModeling::getCombinatorialInputs(output)),
        std::vector<SNLBitTerm*>({input}));
    }
    expectTerms(
      collectTerms(SNLDesignModeling::getCombinatorialOutputs(input)),
      expectedOutputs);

    auto* instance =
      SNLInstance::create(top, gate, NLName(type.getString() + "_gate"));
    ASSERT_NE(nullptr, instance);
    std::vector<SNLInstTerm*> expectedInstOutputs;
    for (auto* output: outputs->getBits()) {
      expectedInstOutputs.push_back(instance->getInstTerm(output));
    }
    expectTerms(
      collectTerms(
        SNLDesignModeling::getCombinatorialOutputs(instance->getInstTerm(input))),
      expectedInstOutputs);

    EXPECT_EQ(gate, NLDB0::getOrCreateNOutputGate(type, 4));
    EXPECT_EQ(
      expectedOutputs.size(),
      SNLDesignModeling::getCombinatorialOutputs(input).size());
  }
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
  EXPECT_EQ(1, mux2->getID());
  ASSERT_NE(nullptr, mux2->getLibrary());
  EXPECT_EQ(2, mux2->getLibrary()->getID());
  EXPECT_EQ(NLName("naja_mux2"), mux2->getLibrary()->getName());
  EXPECT_EQ(NLDB0::getDB0RootLibrary(), mux2->getLibrary()->getParentLibrary());
  EXPECT_EQ(mux2, mux2->getLibrary()->getSNLDesign(NLID::DesignID(1)));
  auto* inA = NLDB0::getMux2InputA(mux2);
  auto* inB = NLDB0::getMux2InputB(mux2);
  auto* sel = NLDB0::getMux2Select(mux2);
  auto* out = NLDB0::getMux2Output(mux2);
  ASSERT_NE(nullptr, inA);
  ASSERT_NE(nullptr, inB);
  ASSERT_NE(nullptr, sel);
  ASSERT_NE(nullptr, out);
  EXPECT_EQ(0, inA->getID());
  EXPECT_EQ(1, inB->getID());
  EXPECT_EQ(2, sel->getID());
  EXPECT_EQ(3, out->getID());
  EXPECT_EQ(1, inA->getWidth());
  EXPECT_EQ(1, inB->getWidth());
  EXPECT_EQ(1, out->getWidth());
  EXPECT_EQ(NLName("A"), inA->getName());
  EXPECT_EQ(NLName("B"), inB->getName());
  EXPECT_EQ(NLName("S"), sel->getName());
  EXPECT_EQ(NLName("Y"), out->getName());

  auto tt = NLDB0::getPrimitiveTruthTable(mux2);
  EXPECT_EQ(tt, SNLDesignModeling::getTruthTable(mux2));
  EXPECT_TRUE(SNLDesignModeling::isMux(mux2));
  EXPECT_TRUE(mux2->isMux());
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
  EXPECT_EQ(32, mux232->getID());
  EXPECT_EQ(mux2->getLibrary(), mux232->getLibrary());
  EXPECT_EQ(mux232, mux232->getLibrary()->getSNLDesign(NLID::DesignID(32)));
  EXPECT_EQ(tt, NLDB0::getPrimitiveTruthTable(mux232));
  EXPECT_TRUE(SNLDesignModeling::isMux(mux232));
  EXPECT_TRUE(mux232->isMux());
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
  EXPECT_EQ(0, mux232A->getID());
  EXPECT_EQ(1, mux232B->getID());
  EXPECT_EQ(2, mux232S->getID());
  EXPECT_EQ(3, mux232Y->getID());
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

TEST_F(NLDB0Test, testTableSelectModelingArcsAreBitLanePrecise) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  NLDB0::TableSelectSignature signature;
  signature.width = 4;
  signature.depth = 3;
  signature.abits = 2;
  auto* tableSelect = NLDB0::getOrCreateTableSelect(signature);
  ASSERT_NE(nullptr, tableSelect);

  auto* data = NLDB0::getTableSelectData(tableSelect);
  auto* addr = NLDB0::getTableSelectAddress(tableSelect);
  auto* out = NLDB0::getTableSelectOutput(tableSelect);
  ASSERT_NE(nullptr, data);
  ASSERT_NE(nullptr, addr);
  ASSERT_NE(nullptr, out);

  for (size_t bit = 0; bit < signature.width; ++bit) {
    auto* yBit = out->getBit(static_cast<NLID::Bit>(bit));
    ASSERT_NE(nullptr, yBit);

    auto inputs = std::vector(
      SNLDesignModeling::getCombinatorialInputs(yBit).begin(),
      SNLDesignModeling::getCombinatorialInputs(yBit).end());
    EXPECT_EQ(signature.abits + signature.depth, inputs.size());
    for (size_t addrBit = 0; addrBit < signature.abits; ++addrBit) {
      EXPECT_NE(
        inputs.end(),
        std::find(
          inputs.begin(),
          inputs.end(),
          addr->getBit(static_cast<NLID::Bit>(addrBit))));
    }
    for (size_t row = 0; row < signature.depth; ++row) {
      EXPECT_NE(
        inputs.end(),
        std::find(
          inputs.begin(),
          inputs.end(),
          data->getBit(static_cast<NLID::Bit>(row * signature.width + bit))));
      EXPECT_EQ(
        inputs.end(),
        std::find(
          inputs.begin(),
          inputs.end(),
          data->getBit(static_cast<NLID::Bit>(
            row * signature.width + ((bit + 1) % signature.width)))));
    }
  }

  for (size_t bit = 0; bit < signature.width; ++bit) {
    auto* dataBit = data->getBit(static_cast<NLID::Bit>(bit));
    ASSERT_NE(nullptr, dataBit);
    auto outputs = std::vector(
      SNLDesignModeling::getCombinatorialOutputs(dataBit).begin(),
      SNLDesignModeling::getCombinatorialOutputs(dataBit).end());
    EXPECT_EQ(1u, outputs.size());
    EXPECT_EQ(out->getBit(static_cast<NLID::Bit>(bit)), outputs.front());
  }

  for (size_t addrBit = 0; addrBit < signature.abits; ++addrBit) {
    auto* addressBit = addr->getBit(static_cast<NLID::Bit>(addrBit));
    ASSERT_NE(nullptr, addressBit);
    auto outputs = std::vector(
      SNLDesignModeling::getCombinatorialOutputs(addressBit).begin(),
      SNLDesignModeling::getCombinatorialOutputs(addressBit).end());
    EXPECT_EQ(signature.width, outputs.size());
    for (size_t bit = 0; bit < signature.width; ++bit) {
      EXPECT_NE(
        outputs.end(),
        std::find(
          outputs.begin(),
          outputs.end(),
          out->getBit(static_cast<NLID::Bit>(bit))));
    }
  }
}

TEST_F(NLDB0Test, testTableSelectSignatureAndErrors) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  NLDB0::TableSelectSignature signature;
  signature.width = 4;
  signature.depth = 3;
  signature.abits = 2;
  auto* tableSelect = NLDB0::getOrCreateTableSelect(signature);
  ASSERT_NE(nullptr, tableSelect);
  EXPECT_EQ(signature, NLDB0::getTableSelectSignature(tableSelect));

  auto* db = NLDB::create(NLUniverse::get());
  ASSERT_NE(nullptr, db);
  auto* library =
      NLLibrary::create(db, NLLibrary::Type::Standard, NLName("designs"));
  ASSERT_NE(nullptr, library);
  auto* top =
      SNLDesign::create(library, SNLDesign::Type::Standard, NLName("top"));
  ASSERT_NE(nullptr, top);
  auto* instance = SNLInstance::create(top, tableSelect, NLName("select0"));
  ASSERT_NE(nullptr, instance);
  EXPECT_EQ(signature, NLDB0::getTableSelectSignature(instance));

  SNLInstParameter::create(instance, tableSelect->getParameter(NLName("WIDTH")), "8");
  SNLInstParameter::create(instance, tableSelect->getParameter(NLName("DEPTH")), "5");
  SNLInstParameter::create(instance, tableSelect->getParameter(NLName("ABITS")), "3");
  auto overridden = NLDB0::getTableSelectSignature(instance);
  EXPECT_EQ(8, overridden.width);
  EXPECT_EQ(5, overridden.depth);
  EXPECT_EQ(3, overridden.abits);

  EXPECT_THROW(
    NLDB0::getTableSelectSignature(static_cast<const SNLInstance*>(nullptr)),
    NLException);
  auto* notTableSelect = SNLDesign::create(
    library,
    SNLDesign::Type::Standard,
    NLName("notTableSelect"));
  ASSERT_NE(nullptr, notTableSelect);
  auto* notTableSelectInstance = SNLInstance::create(top, notTableSelect, NLName("notSelect0"));
  ASSERT_NE(nullptr, notTableSelectInstance);
  EXPECT_FALSE(NLDB0::isTableSelect(notTableSelect));
  EXPECT_THROW(NLDB0::getTableSelectSignature(notTableSelect), NLException);
  EXPECT_THROW(NLDB0::getTableSelectSignature(notTableSelectInstance), NLException);
  EXPECT_THROW(NLDB0::getTableSelectData(notTableSelect), NLException);
  EXPECT_THROW(NLDB0::getTableSelectAddress(notTableSelect), NLException);
  EXPECT_THROW(NLDB0::getTableSelectOutput(notTableSelect), NLException);

  NLDB0::TableSelectSignature invalidSignature;
  invalidSignature.width = 0;
  invalidSignature.depth = 1;
  invalidSignature.abits = 1;
  EXPECT_THROW(NLDB0::getOrCreateTableSelect(invalidSignature), NLException);

  invalidSignature.width = 2;
  invalidSignature.depth = std::numeric_limits<size_t>::max() / 2 + 1;
  invalidSignature.abits = 1;
  EXPECT_THROW(NLDB0::getOrCreateTableSelect(invalidSignature), NLException);

  invalidSignature.width = static_cast<size_t>(std::numeric_limits<NLID::Bit>::max()) + 2;
  invalidSignature.depth = 1;
  invalidSignature.abits = 1;
  EXPECT_THROW(NLDB0::getOrCreateTableSelect(invalidSignature), NLException);
}

TEST_F(NLDB0Test, testClonePreservesDB0WideMux2Model) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto* mux24 = NLDB0::getOrCreateMux2(4);
  ASSERT_NE(nullptr, mux24);

  auto* db = NLDB::create(NLUniverse::get());
  ASSERT_NE(nullptr, db);
  auto* library =
      NLLibrary::create(db, NLLibrary::Type::Standard, NLName("designs"));
  ASSERT_NE(nullptr, library);

  auto* top =
      SNLDesign::create(library, SNLDesign::Type::Standard, NLName("top"));
  ASSERT_NE(nullptr, top);
  auto* muxInstance = SNLInstance::create(top, mux24, NLName("mux0"));
  ASSERT_NE(nullptr, muxInstance);

  auto* topClone = top->clone(NLName("topClone"));
  ASSERT_NE(nullptr, topClone);
  auto* clonedMuxInstance = topClone->getInstance(NLName("mux0"));
  ASSERT_NE(nullptr, clonedMuxInstance);
  EXPECT_EQ(mux24, clonedMuxInstance->getModel());
  EXPECT_TRUE(NLDB0::isDB0Primitive(clonedMuxInstance->getModel()));
  EXPECT_TRUE(NLDB0::isMux2(clonedMuxInstance->getModel()));
}

TEST_F(NLDB0Test, testDivModPrimitive) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  NLDB0::DivModSignature signature;
  signature.width = 8;
  signature.isSigned = false;

  auto* divmod0 = NLDB0::getOrCreateDivMod(signature);
  ASSERT_NE(nullptr, divmod0);
  EXPECT_TRUE(divmod0->isPrimitive());
  EXPECT_TRUE(NLDB0::isDB0Primitive(divmod0));
  EXPECT_TRUE(NLDB0::isDivMod(divmod0));
  ASSERT_NE(nullptr, divmod0->getLibrary());
  EXPECT_EQ(8, divmod0->getID());
  EXPECT_EQ(NLName("unsigned"), divmod0->getLibrary()->getName());
  EXPECT_EQ(13, divmod0->getLibrary()->getID());
  ASSERT_NE(nullptr, divmod0->getLibrary()->getParentLibrary());
  EXPECT_EQ(NLName("naja_divmod"), divmod0->getLibrary()->getParentLibrary()->getName());
  EXPECT_EQ(12, divmod0->getLibrary()->getParentLibrary()->getID());
  EXPECT_EQ(NLName("naja_divmod__u_w8"), divmod0->getName());
  EXPECT_EQ(divmod0, divmod0->getLibrary()->getSNLDesign(NLID::DesignID(8)));
  EXPECT_EQ(signature, NLDB0::getDivModSignature(divmod0));
  EXPECT_EQ("8", divmod0->getParameter(NLName("WIDTH"))->getValue());
  EXPECT_EQ("0", divmod0->getParameter(NLName("SIGNED"))->getValue());

  auto* divmod1 = NLDB0::getOrCreateDivMod(signature);
  EXPECT_EQ(divmod0, divmod1);

  NLDB0::DivModSignature signedSignature;
  signedSignature.width = 8;
  signedSignature.isSigned = true;
  auto* signedDivMod = NLDB0::getOrCreateDivMod(signedSignature);
  ASSERT_NE(nullptr, signedDivMod);
  EXPECT_NE(divmod0, signedDivMod);
  EXPECT_EQ(8, signedDivMod->getID());
  ASSERT_NE(nullptr, signedDivMod->getLibrary());
  EXPECT_EQ(NLName("signed"), signedDivMod->getLibrary()->getName());
  EXPECT_EQ(14, signedDivMod->getLibrary()->getID());
  EXPECT_EQ(divmod0->getLibrary()->getParentLibrary(), signedDivMod->getLibrary()->getParentLibrary());
  EXPECT_EQ(signedDivMod, signedDivMod->getLibrary()->getSNLDesign(NLID::DesignID(8)));
  EXPECT_EQ(NLName("naja_divmod__s_w8"), signedDivMod->getName());
  EXPECT_EQ(signedSignature, NLDB0::getDivModSignature(signedDivMod));
  EXPECT_EQ("1", signedDivMod->getParameter(NLName("SIGNED"))->getValue());

  auto* dividend = NLDB0::getDivModDividend(divmod0);
  auto* divisor = NLDB0::getDivModDivisor(divmod0);
  auto* quotient = NLDB0::getDivModQuotient(divmod0);
  auto* remainder = NLDB0::getDivModRemainder(divmod0);
  ASSERT_NE(nullptr, dividend);
  ASSERT_NE(nullptr, divisor);
  ASSERT_NE(nullptr, quotient);
  ASSERT_NE(nullptr, remainder);
  EXPECT_EQ(0, dividend->getID());
  EXPECT_EQ(1, divisor->getID());
  EXPECT_EQ(2, quotient->getID());
  EXPECT_EQ(3, remainder->getID());
  EXPECT_EQ(SNLTerm::Direction::Input, dividend->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, divisor->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, quotient->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, remainder->getDirection());
  EXPECT_EQ(8, dividend->getWidth());
  EXPECT_EQ(8, divisor->getWidth());
  EXPECT_EQ(8, quotient->getWidth());
  EXPECT_EQ(8, remainder->getWidth());

  EXPECT_THROW(NLDB0::getPrimitiveTruthTable(divmod0), NLException);
  EXPECT_TRUE(SNLDesignModeling::hasModeling(divmod0));
  auto q0Inputs = std::vector(
    SNLDesignModeling::getCombinatorialInputs(
      static_cast<SNLBitTerm*>(quotient->getBit(0))).begin(),
    SNLDesignModeling::getCombinatorialInputs(
      static_cast<SNLBitTerm*>(quotient->getBit(0))).end());
  EXPECT_EQ(16u, q0Inputs.size());
  EXPECT_NE(q0Inputs.end(), std::find(q0Inputs.begin(), q0Inputs.end(), dividend->getBit(0)));
  EXPECT_NE(q0Inputs.end(), std::find(q0Inputs.begin(), q0Inputs.end(), divisor->getBit(0)));

  //Host the test design in a regular user DB: DB0 is reserved for NLDB0-managed
  //primitives, which always use explicit library IDs.
  auto* db = NLDB::create(NLUniverse::get());
  ASSERT_NE(nullptr, db);
  auto* topLibrary = NLLibrary::create(db, NLName("WORK"));
  auto* top = SNLDesign::create(topLibrary, SNLDesign::Type::Standard, NLName("top"));
  auto* inst = SNLInstance::create(top, divmod0, NLName("div0"));
  ASSERT_NE(nullptr, inst);
  EXPECT_EQ(signature, NLDB0::getDivModSignature(inst));
  SNLInstParameter::create(inst, divmod0->getParameter(NLName("WIDTH")), "4");
  SNLInstParameter::create(inst, divmod0->getParameter(NLName("SIGNED")), "1");
  const auto overridden = NLDB0::getDivModSignature(inst);
  EXPECT_EQ(4u, overridden.width);
  EXPECT_TRUE(overridden.isSigned);

  EXPECT_THROW(
      NLDB0::getDivModSignature(static_cast<const SNLInstance*>(nullptr)),
      NLException);

  auto* notDivMod = SNLDesign::create(
      topLibrary,
      SNLDesign::Type::Standard,
      NLName("not_divmod"));
  ASSERT_NE(nullptr, notDivMod);
  EXPECT_FALSE(NLDB0::isDivMod(notDivMod));
  EXPECT_THROW(NLDB0::getDivModSignature(notDivMod), NLException);
  EXPECT_THROW(NLDB0::getDivModDividend(notDivMod), NLException);
  EXPECT_THROW(NLDB0::getDivModDivisor(notDivMod), NLException);
  EXPECT_THROW(NLDB0::getDivModQuotient(notDivMod), NLException);
  EXPECT_THROW(NLDB0::getDivModRemainder(notDivMod), NLException);

  auto* malformed = SNLDesign::create(
      divmod0->getLibrary(),
      SNLDesign::Type::Primitive,
      NLName("naja_divmod__missing_width"));
  ASSERT_TRUE(NLDB0::isDivMod(malformed));
  EXPECT_THROW(NLDB0::getDivModSignature(malformed), NLException);
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
  ASSERT_NE(nullptr, memory0->getLibrary());
  EXPECT_EQ(NLName("MEMORY"), memory0->getLibrary()->getName());
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

  //Host the test design in a regular user DB: DB0 is reserved for NLDB0-managed
  //primitives, which always use explicit library IDs.
  auto* db = NLDB::create(NLUniverse::get());
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
  EXPECT_EQ(signatureTemplate, NLDB0::getMemorySignature(memory));

  auto* inst = SNLInstance::create(top, memory, NLName("mem0"));
  ASSERT_NE(nullptr, inst);
  EXPECT_EQ(signatureTemplate, NLDB0::getMemorySignature(inst));
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

  const auto instInterface = SNLDesignModeling::getMemoryInterface(inst);
  EXPECT_EQ(7u, instInterface.width);
  EXPECT_EQ(4u, instInterface.depth);
  EXPECT_EQ(2u, instInterface.abits);
  EXPECT_EQ(SNLDesignModeling::MemoryResetMode::SyncLow, instInterface.resetMode);
  EXPECT_EQ(nullptr, instInterface.clock);
  EXPECT_TRUE(instInterface.readPorts.empty());
  EXPECT_TRUE(instInterface.writePorts.empty());

  EXPECT_THROW(
      NLDB0::getMemorySignature(static_cast<const SNLInstance*>(nullptr)),
      NLException);
}

TEST_F(NLDB0Test, testMemoryPrimitiveSyncResetModes) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto checkMemory = [](NLDB0::MemoryResetMode mode,
                        const char* expectedName,
                        const char* expectedEnable,
                        const char* expectedAsync,
                        const char* expectedActiveLow,
                        SNLDesignModeling::MemoryResetMode expectedModelingMode) {
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
    EXPECT_EQ(expectedEnable, memory->getParameter(NLName("RST_ENABLE"))->getValue());
    EXPECT_EQ(expectedAsync, memory->getParameter(NLName("RST_ASYNC"))->getValue());
    EXPECT_EQ(expectedActiveLow, memory->getParameter(NLName("RST_ACTIVE_LOW"))->getValue());
    EXPECT_EQ(expectedModelingMode, SNLDesignModeling::getMemoryInterface(memory).resetMode);
  };

  checkMemory(
    NLDB0::MemoryResetMode::None,
    "naja_mem__w4_d8_a3_r1_w1_rst_none",
    "0",
    "0",
    "0",
    SNLDesignModeling::MemoryResetMode::None);
  checkMemory(
    NLDB0::MemoryResetMode::AsyncHigh,
    "naja_mem__w4_d8_a3_r1_w1_rst_async_high",
    "1",
    "1",
    "0",
    SNLDesignModeling::MemoryResetMode::AsyncHigh);
  checkMemory(
    NLDB0::MemoryResetMode::SyncLow,
    "naja_mem__w4_d8_a3_r1_w1_rst_sync_low",
    "1",
    "0",
    "1",
    SNLDesignModeling::MemoryResetMode::SyncLow);
  checkMemory(
    NLDB0::MemoryResetMode::SyncHigh,
    "naja_mem__w4_d8_a3_r1_w1_rst_sync_high",
    "1",
    "0",
    "0",
    SNLDesignModeling::MemoryResetMode::SyncHigh);
}

TEST_F(NLDB0Test, testLazyPrimitiveLibraryRecreationAndWidthErrors) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto* rootLibrary = NLDB0::getDB0RootLibrary();
  ASSERT_NE(nullptr, rootLibrary);

  auto* dffLibrary = rootLibrary->getLibrary(expectedSequentialLibraryID(NLName("naja_dff")));
  ASSERT_NE(nullptr, dffLibrary);
  dffLibrary->destroy();
  EXPECT_EQ(
      nullptr,
      rootLibrary->getLibrary(expectedSequentialLibraryID(NLName("naja_dff"))));

  auto* dff2 = NLDB0::getOrCreateDFF(2);
  ASSERT_NE(nullptr, dff2);
  EXPECT_TRUE(NLDB0::isDFF(dff2));
  ASSERT_NE(nullptr, dff2->getLibrary());
  EXPECT_EQ(expectedSequentialLibraryID(NLName("naja_dff")), dff2->getLibrary()->getID());
  EXPECT_EQ(NLName("naja_dff"), dff2->getLibrary()->getName());

  auto* divModLibrary = rootLibrary->getLibrary(NLID::LibraryID(12));
  ASSERT_NE(nullptr, divModLibrary);
  auto* unsignedDivModLibrary = divModLibrary->getLibrary(NLID::LibraryID(13));
  ASSERT_NE(nullptr, unsignedDivModLibrary);
  unsignedDivModLibrary->destroy();
  EXPECT_EQ(nullptr, divModLibrary->getLibrary(NLID::LibraryID(13)));

  NLDB0::DivModSignature divModSignature;
  divModSignature.width = 4;
  auto* divMod = NLDB0::getOrCreateDivMod(divModSignature);
  ASSERT_NE(nullptr, divMod);
  ASSERT_NE(nullptr, divMod->getLibrary());
  EXPECT_TRUE(NLDB0::isDivMod(divMod));
  EXPECT_EQ(NLID::LibraryID(13), divMod->getLibrary()->getID());
  EXPECT_EQ(NLName("unsigned"), divMod->getLibrary()->getName());
  EXPECT_EQ(divModLibrary, divMod->getLibrary()->getParentLibrary());

  auto* tableSelectLibrary = rootLibrary->getLibrary(NLID::LibraryID(15));
  ASSERT_NE(nullptr, tableSelectLibrary);
  tableSelectLibrary->destroy();
  EXPECT_EQ(nullptr, rootLibrary->getLibrary(NLID::LibraryID(15)));

  NLDB0::TableSelectSignature tableSelectSignature;
  tableSelectSignature.width = 4;
  tableSelectSignature.depth = 3;
  tableSelectSignature.abits = 2;
  auto* tableSelect = NLDB0::getOrCreateTableSelect(tableSelectSignature);
  ASSERT_NE(nullptr, tableSelect);
  ASSERT_NE(nullptr, tableSelect->getLibrary());
  EXPECT_TRUE(NLDB0::isTableSelect(tableSelect));
  EXPECT_EQ(NLID::LibraryID(15), tableSelect->getLibrary()->getID());
  EXPECT_EQ(NLName("naja_table_select"), tableSelect->getLibrary()->getName());
  EXPECT_EQ(rootLibrary, tableSelect->getLibrary()->getParentLibrary());
  EXPECT_THROW(NLDB0::getPrimitiveTruthTable(tableSelect), NLException);

  EXPECT_THROW(NLDB0::getOrCreateDFF(0), NLException);
  const auto tooWide =
      static_cast<size_t>(std::numeric_limits<NLID::DesignID>::max()) + 1;
  EXPECT_THROW(NLDB0::getOrCreateDFF(tooWide), NLException);
}

TEST_F(NLDB0Test, testMalformedDB0MemorySignatureThrows) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  NLDB0::MemorySignature signature;
  signature.width = 4;
  signature.depth = 8;
  signature.abits = 3;
  signature.readPorts = 1;
  signature.writePorts = 1;

  auto* memory = NLDB0::getOrCreateMemory(signature);
  ASSERT_NE(nullptr, memory);
  auto* memoryLibrary = memory->getLibrary();
  ASSERT_NE(nullptr, memoryLibrary);

  auto* malformed = SNLDesign::create(
      memoryLibrary,
      SNLDesign::Type::Primitive,
      NLName("naja_mem__missing_width"));
  ASSERT_TRUE(NLDB0::isMemory(malformed));
  EXPECT_THROW(NLDB0::getMemorySignature(malformed), NLException);
}

TEST_F(NLDB0Test, testMemoryRecognitionIsScopedToDB0MemoryLibrary) {
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

  SNLDesignModeling::MemoryInterface interface;
  interface.width = 8;
  interface.depth = 4;
  interface.abits = 2;
  interface.clock = clk;
  interface.reset = rst;
  interface.resetMode = SNLDesignModeling::MemoryResetMode::AsyncLow;
  interface.readPorts.push_back(
      {.address = {static_cast<SNLBitTerm*>(raddr->getBit(0)),
                   static_cast<SNLBitTerm*>(raddr->getBit(1))},
       .data = {static_cast<SNLBitTerm*>(rdata->getBit(0)),
                static_cast<SNLBitTerm*>(rdata->getBit(1)),
                static_cast<SNLBitTerm*>(rdata->getBit(2)),
                static_cast<SNLBitTerm*>(rdata->getBit(3)),
                static_cast<SNLBitTerm*>(rdata->getBit(4)),
                static_cast<SNLBitTerm*>(rdata->getBit(5)),
                static_cast<SNLBitTerm*>(rdata->getBit(6)),
                static_cast<SNLBitTerm*>(rdata->getBit(7))}});
  interface.writePorts.push_back(
      {.address = {static_cast<SNLBitTerm*>(waddr->getBit(0)),
                   static_cast<SNLBitTerm*>(waddr->getBit(1))},
       .data = {static_cast<SNLBitTerm*>(wdata->getBit(0)),
                static_cast<SNLBitTerm*>(wdata->getBit(1)),
                static_cast<SNLBitTerm*>(wdata->getBit(2)),
                static_cast<SNLBitTerm*>(wdata->getBit(3)),
                static_cast<SNLBitTerm*>(wdata->getBit(4)),
                static_cast<SNLBitTerm*>(wdata->getBit(5)),
                static_cast<SNLBitTerm*>(wdata->getBit(6)),
                static_cast<SNLBitTerm*>(wdata->getBit(7))},
       .enables = {static_cast<SNLBitTerm*>(we->getBit(0))}});
  SNLDesignModeling::setMemoryInterface(memory, interface);

  EXPECT_TRUE(SNLDesignModeling::hasMemoryInterface(memory));
  EXPECT_FALSE(NLDB0::isMemory(memory));
  EXPECT_THROW(NLDB0::getMemorySignature(memory), NLException);
  EXPECT_THROW(NLDB0::getMemoryClock(memory), NLException);
  EXPECT_THROW(NLDB0::getMemoryReset(memory), NLException);
  EXPECT_THROW(NLDB0::getMemoryReadAddress(memory), NLException);
  EXPECT_THROW(NLDB0::getMemoryReadData(memory), NLException);
  EXPECT_THROW(NLDB0::getMemoryWriteAddress(memory), NLException);
  EXPECT_THROW(NLDB0::getMemoryWriteData(memory), NLException);
  EXPECT_THROW(NLDB0::getMemoryWriteEnable(memory), NLException);

  auto* designs = NLLibrary::create(db, NLName("designs"));
  auto* top = SNLDesign::create(designs, NLName("top"));
  auto* inst = SNLInstance::create(top, memory, NLName("non_db0_mem"));
  EXPECT_THROW(NLDB0::getMemorySignature(inst), NLException);
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
      NLDB0::getDFFR(),
      NLDB0::getDFFRClock(),
      {NLDB0::getDFFRData(), NLDB0::getDFFRReset()},
      {NLDB0::getDFFROutput()},
    },
    {
      NLDB0::getDFFS(),
      NLDB0::getDFFSClock(),
      {NLDB0::getDFFSData(), NLDB0::getDFFSSet()},
      {NLDB0::getDFFSOutput()},
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
    {
      NLDB0::getDFFSR(),
      NLDB0::getDFFSRClock(),
      {NLDB0::getDFFSRData(), NLDB0::getDFFSRReset()},
      {NLDB0::getDFFSROutput()},
    },
    {
      NLDB0::getDFFSRN(),
      NLDB0::getDFFSRNClock(),
      {NLDB0::getDFFSRNData(), NLDB0::getDFFSRNResetN()},
      {NLDB0::getDFFSRNOutput()},
    },
    {
      NLDB0::getDFFSS(),
      NLDB0::getDFFSSClock(),
      {NLDB0::getDFFSSData(), NLDB0::getDFFSSSet()},
      {NLDB0::getDFFSSOutput()},
    },
    {
      NLDB0::getDFFSSN(),
      NLDB0::getDFFSSNClock(),
      {NLDB0::getDFFSSNData(), NLDB0::getDFFSSNSetN()},
      {NLDB0::getDFFSSNOutput()},
    },
    {
      NLDB0::getDFFSRE(),
      NLDB0::getDFFSRE()->getScalarTerm(NLID::DesignObjectID(0)),
      {
        NLDB0::getDFFSRE()->getScalarTerm(NLID::DesignObjectID(1)),
        NLDB0::getDFFSRE()->getScalarTerm(NLID::DesignObjectID(2)),
        NLDB0::getDFFSRE()->getScalarTerm(NLID::DesignObjectID(3)),
      },
      {NLDB0::getDFFSRE()->getScalarTerm(NLID::DesignObjectID(4))},
    },
    {
      NLDB0::getDFFSRNE(),
      NLDB0::getDFFSRNE()->getScalarTerm(NLID::DesignObjectID(0)),
      {
        NLDB0::getDFFSRNE()->getScalarTerm(NLID::DesignObjectID(1)),
        NLDB0::getDFFSRNE()->getScalarTerm(NLID::DesignObjectID(2)),
        NLDB0::getDFFSRNE()->getScalarTerm(NLID::DesignObjectID(3)),
      },
      {NLDB0::getDFFSRNE()->getScalarTerm(NLID::DesignObjectID(4))},
    },
    {
      NLDB0::getDFFSSE(),
      NLDB0::getDFFSSE()->getScalarTerm(NLID::DesignObjectID(0)),
      {
        NLDB0::getDFFSSE()->getScalarTerm(NLID::DesignObjectID(1)),
        NLDB0::getDFFSSE()->getScalarTerm(NLID::DesignObjectID(2)),
        NLDB0::getDFFSSE()->getScalarTerm(NLID::DesignObjectID(3)),
      },
      {NLDB0::getDFFSSE()->getScalarTerm(NLID::DesignObjectID(4))},
    },
    {
      NLDB0::getDFFSSNE(),
      NLDB0::getDFFSSNE()->getScalarTerm(NLID::DesignObjectID(0)),
      {
        NLDB0::getDFFSSNE()->getScalarTerm(NLID::DesignObjectID(1)),
        NLDB0::getDFFSSNE()->getScalarTerm(NLID::DesignObjectID(2)),
        NLDB0::getDFFSSNE()->getScalarTerm(NLID::DesignObjectID(3)),
      },
      {NLDB0::getDFFSSNE()->getScalarTerm(NLID::DesignObjectID(4))},
    },
  };

  for (const auto& primitive: sequentialPrimitives) {
    ASSERT_NE(nullptr, primitive.design);
    ASSERT_NE(nullptr, primitive.clock);
    EXPECT_TRUE(NLDB0::isDB0Primitive(primitive.design));
    EXPECT_EQ(1, primitive.design->getID());
    ASSERT_NE(nullptr, primitive.design->getLibrary());
    EXPECT_EQ(expectedSequentialLibraryID(primitive.design->getName()), primitive.design->getLibrary()->getID());
    EXPECT_EQ(primitive.design->getName(), primitive.design->getLibrary()->getName());
    EXPECT_EQ(NLDB0::getDB0RootLibrary(), primitive.design->getLibrary()->getParentLibrary());
    EXPECT_EQ(
      primitive.design,
      primitive.design->getLibrary()->getSNLDesign(NLID::DesignID(1)));
    EXPECT_EQ(0, primitive.clock->getID());
    for (size_t i = 0; i < primitive.inputs.size(); ++i) {
      EXPECT_EQ(i + 1, primitive.inputs[i]->getID());
    }
    for (size_t i = 0; i < primitive.outputs.size(); ++i) {
      EXPECT_EQ(primitive.inputs.size() + 1 + i, primitive.outputs[i]->getID());
    }
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

TEST_F(NLDB0Test, testDFFR) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto dffr = NLDB0::getDFFR();
  ASSERT_NE(nullptr, dffr);
  EXPECT_TRUE(NLDB0::isDFFR(dffr));
  EXPECT_TRUE(NLDB0::isDB0Primitive(dffr));
  EXPECT_EQ(NLName("naja_dffr"), dffr->getName());
  EXPECT_FALSE(NLDB0::isDFFR(NLDB0::getDFF()));
  EXPECT_FALSE(NLDB0::isDFFR(NLDB0::getDFFRN()));

  auto clock = NLDB0::getDFFRClock();
  auto data = NLDB0::getDFFRData();
  auto reset = NLDB0::getDFFRReset();
  auto output = NLDB0::getDFFROutput();
  ASSERT_NE(nullptr, clock);
  ASSERT_NE(nullptr, data);
  ASSERT_NE(nullptr, reset);
  ASSERT_NE(nullptr, output);
  EXPECT_EQ(NLName("C"), clock->getName());
  EXPECT_EQ(NLName("D"), data->getName());
  EXPECT_EQ(NLName("R"), reset->getName());
  EXPECT_EQ(NLName("Q"), output->getName());
  EXPECT_EQ(SNLTerm::Direction::Input, clock->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, data->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, reset->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, output->getDirection());

  auto dffr4 = NLDB0::getOrCreateDFFR(4);
  ASSERT_NE(nullptr, dffr4);
  EXPECT_TRUE(NLDB0::isDFFR(dffr4));
}

TEST_F(NLDB0Test, testDFFS) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto dffs = NLDB0::getDFFS();
  ASSERT_NE(nullptr, dffs);
  EXPECT_TRUE(NLDB0::isDFFS(dffs));
  EXPECT_TRUE(NLDB0::isDB0Primitive(dffs));
  EXPECT_EQ(NLName("naja_dffs"), dffs->getName());
  EXPECT_FALSE(NLDB0::isDFFS(NLDB0::getDFF()));
  EXPECT_FALSE(NLDB0::isDFFS(NLDB0::getDFFR()));

  auto clock = NLDB0::getDFFSClock();
  auto data = NLDB0::getDFFSData();
  auto set = NLDB0::getDFFSSet();
  auto output = NLDB0::getDFFSOutput();
  ASSERT_NE(nullptr, clock);
  ASSERT_NE(nullptr, data);
  ASSERT_NE(nullptr, set);
  ASSERT_NE(nullptr, output);
  EXPECT_EQ(NLName("C"), clock->getName());
  EXPECT_EQ(NLName("D"), data->getName());
  EXPECT_EQ(NLName("S"), set->getName());
  EXPECT_EQ(NLName("Q"), output->getName());
  EXPECT_EQ(SNLTerm::Direction::Input, clock->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, data->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, set->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, output->getDirection());

  auto dffs4 = NLDB0::getOrCreateDFFS(4);
  ASSERT_NE(nullptr, dffs4);
  EXPECT_TRUE(NLDB0::isDFFS(dffs4));
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

TEST_F(NLDB0Test, testSyncDFFWidthPrimitives) {
  NLUniverse::create();
  using Role = SNLDesignModeling::SNLTermRole;
  using Level = SNLDesignModeling::SNLActiveLevel;

  auto* dffsr4 = NLDB0::getOrCreateDFFSR(4);
  ASSERT_NE(nullptr, dffsr4);
  EXPECT_TRUE(NLDB0::isDFFSR(dffsr4));
  EXPECT_EQ(NLName("naja_dffsr__w4"), dffsr4->getName());
  EXPECT_EQ(NLName("C"), dffsr4->getScalarTerm(NLName("C"))->getName());
  EXPECT_EQ(NLName("D"), dffsr4->getBusTerm(NLName("D"))->getName());
  EXPECT_EQ(NLName("R"), dffsr4->getScalarTerm(NLName("R"))->getName());
  EXPECT_EQ(NLName("Q"), dffsr4->getBusTerm(NLName("Q"))->getName());
  EXPECT_EQ(Role::SyncReset,
            SNLDesignModeling::getTermRole(dffsr4->getScalarTerm(NLName("R"))));
  EXPECT_EQ(Level::High,
            SNLDesignModeling::getResetActiveLevel(dffsr4->getScalarTerm(NLName("R"))));

  auto* dffsrne4 = NLDB0::getOrCreateDFFSRNE(4);
  ASSERT_NE(nullptr, dffsrne4);
  EXPECT_TRUE(NLDB0::isDFFSRNE(dffsrne4));
  EXPECT_EQ(NLName("naja_dffsrne__w4"), dffsrne4->getName());
  EXPECT_EQ(NLName("E"), dffsrne4->getScalarTerm(NLName("E"))->getName());
  EXPECT_EQ(NLName("RN"), dffsrne4->getScalarTerm(NLName("RN"))->getName());
  EXPECT_EQ(Role::Enable,
            SNLDesignModeling::getTermRole(dffsrne4->getScalarTerm(NLName("E"))));
  EXPECT_EQ(Role::SyncReset,
            SNLDesignModeling::getTermRole(dffsrne4->getScalarTerm(NLName("RN"))));
  EXPECT_EQ(Level::Low,
            SNLDesignModeling::getResetActiveLevel(dffsrne4->getScalarTerm(NLName("RN"))));

  auto* dffssne2 = NLDB0::getOrCreateDFFSSNE(2);
  ASSERT_NE(nullptr, dffssne2);
  EXPECT_TRUE(NLDB0::isDFFSSNE(dffssne2));
  EXPECT_EQ(NLName("SN"), dffssne2->getScalarTerm(NLName("SN"))->getName());
  EXPECT_EQ(Role::SyncSet,
            SNLDesignModeling::getTermRole(dffssne2->getScalarTerm(NLName("SN"))));
  EXPECT_EQ(Level::Low,
            SNLDesignModeling::getResetActiveLevel(dffssne2->getScalarTerm(NLName("SN"))));
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
  EXPECT_EQ(nullptr, NLDB0::getDFFR());
  EXPECT_FALSE(NLDB0::isDFFR(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getDFFRClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFRData());
  EXPECT_EQ(nullptr, NLDB0::getDFFRReset());
  EXPECT_EQ(nullptr, NLDB0::getDFFROutput());
  EXPECT_EQ(nullptr, NLDB0::getDFFS());
  EXPECT_FALSE(NLDB0::isDFFS(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getDFFSClock());
  EXPECT_EQ(nullptr, NLDB0::getDFFSData());
  EXPECT_EQ(nullptr, NLDB0::getDFFSSet());
  EXPECT_EQ(nullptr, NLDB0::getDFFSOutput());
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
  NLDB0::DivModSignature divModSignature;
  divModSignature.width = 4;
  EXPECT_EQ(nullptr, NLDB0::getOrCreateDivMod(divModSignature));
  NLDB0::TableSelectSignature tableSelectSignature;
  tableSelectSignature.width = 4;
  tableSelectSignature.depth = 3;
  tableSelectSignature.abits = 2;
  EXPECT_EQ(nullptr, NLDB0::getOrCreateTableSelect(tableSelectSignature));
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
  NLDB0::DivModSignature invalidDivModSignature;
  EXPECT_THROW(NLDB0::getOrCreateDivMod(invalidDivModSignature), NLException);
  EXPECT_THROW(NLDB0::getOrCreateMux2(0), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 0), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 0), NLException);
  NLUniverse::get()->destroy();
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2), NLException);
}

TEST_F(NLDB0Test, testGetOrCreatePrimitiveDispatch) {
  NLUniverse::create();

  const auto resolve = [](SNLDesign* primitive) {
    return NLDB0::getOrCreatePrimitive(
        primitive->getLibrary()->getID(), primitive->getID());
  };

  auto* fa = NLDB0::getFA();
  ASSERT_NE(nullptr, fa);
  EXPECT_EQ(fa, resolve(fa));
  EXPECT_EQ(
      nullptr,
      NLDB0::getOrCreatePrimitive(
          fa->getLibrary()->getID(), NLID::DesignID(fa->getID() + 1)));

  const std::vector<SNLDesign*> widthPrimitives {
    NLDB0::getOrCreateMux2(4),
    NLDB0::getOrCreateDFF(4),
    NLDB0::getOrCreateDLatch(4),
    NLDB0::getOrCreateDFFN(4),
    NLDB0::getOrCreateDFFRN(4),
    NLDB0::getOrCreateDFFR(4),
    NLDB0::getOrCreateDFFS(4),
    NLDB0::getOrCreateDFFE(4),
    NLDB0::getOrCreateDFFRE(4),
    NLDB0::getOrCreateDFFSE(4),
    NLDB0::getOrCreateDFFSR(4),
    NLDB0::getOrCreateDFFSRN(4),
    NLDB0::getOrCreateDFFSS(4),
    NLDB0::getOrCreateDFFSSN(4),
    NLDB0::getOrCreateDFFSRE(4),
    NLDB0::getOrCreateDFFSRNE(4),
    NLDB0::getOrCreateDFFSSE(4),
    NLDB0::getOrCreateDFFSSNE(4)
  };
  for (auto* primitive: widthPrimitives) {
    ASSERT_NE(nullptr, primitive);
    EXPECT_EQ(primitive, resolve(primitive));
  }

  for (bool isSigned: {false, true}) {
    NLDB0::DivModSignature signature {4, isSigned};
    auto* divMod = NLDB0::getOrCreateDivMod(signature);
    ASSERT_NE(nullptr, divMod);
    EXPECT_EQ(divMod, resolve(divMod));
  }

  NLDB0::MemorySignature memorySignature {
    8, 16, 4, 2, 1, NLDB0::MemoryResetMode::AsyncLow
  };
  auto* memory = NLDB0::getOrCreateMemory(memorySignature);
  ASSERT_NE(nullptr, memory);
  const NLDB0::PrimitiveParameters memoryParameters {
    {"WIDTH", "8"},
    {"DEPTH", "16"},
    {"ABITS", "4"},
    {"RD_PORTS", "2"},
    {"WR_PORTS", "1"},
    {"RST_ENABLE", "1"},
    {"RST_ASYNC", "1"},
    {"RST_ACTIVE_LOW", "1"}
  };
  EXPECT_EQ(
      memory,
      NLDB0::getOrCreatePrimitive(
          memory->getLibrary()->getID(), memory->getID(), memoryParameters));
  EXPECT_THROW(
      NLDB0::getOrCreatePrimitive(
          memory->getLibrary()->getID(), memory->getID(), {}),
      NLException);

  NLDB0::TableSelectSignature tableSelectSignature {8, 4, 2};
  auto* tableSelect = NLDB0::getOrCreateTableSelect(tableSelectSignature);
  ASSERT_NE(nullptr, tableSelect);
  const NLDB0::PrimitiveParameters tableSelectParameters {
    {"WIDTH", "8"},
    {"DEPTH", "4"},
    {"ABITS", "2"}
  };
  EXPECT_EQ(
      tableSelect,
      NLDB0::getOrCreatePrimitive(
          tableSelect->getLibrary()->getID(),
          tableSelect->getID(),
          tableSelectParameters));

  auto* andGate = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 4);
  auto* buffer = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 4);
  ASSERT_NE(nullptr, andGate);
  ASSERT_NE(nullptr, buffer);
  EXPECT_EQ(andGate, resolve(andGate));
  EXPECT_EQ(buffer, resolve(buffer));

  EXPECT_EQ(
      nullptr,
      NLDB0::getOrCreatePrimitive(
          NLID::LibraryID(std::numeric_limits<NLID::LibraryID>::max()),
          NLID::DesignID(1)));
}
