// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

using namespace naja::NL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif
#ifndef NAJA_DIFF
#define NAJA_DIFF "Undefined"
#endif

//Test assigns
class SNLVRLDumperTest2: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
      SNLDesign* top = SNLDesign::create(library, NLName("top"));

      auto const0 = SNLScalarNet::create(top);
      const0->setType(SNLNet::Type::Assign0);
      auto const1 = SNLScalarNet::create(top);
      const1->setType(SNLNet::Type::Assign1);
      auto bus = SNLBusNet::create(top, -2, 2, NLName("bus"));
      auto scalar = SNLScalarNet::create(top, NLName("scalar"));

      auto assign0 = SNLInstance::create(top, NLDB0::getAssign());
      auto assign1 = SNLInstance::create(top, NLDB0::getAssign());
      auto assign2 = SNLInstance::create(top, NLDB0::getAssign());
      auto assign3 = SNLInstance::create(top, NLDB0::getAssign());
      auto assign4 = SNLInstance::create(top, NLDB0::getAssign());

      assign0->getInstTerm(NLDB0::getAssignInput())->setNet(const0);
      assign0->getInstTerm(NLDB0::getAssignOutput())->setNet(bus->getBit(-2));
      assign1->getInstTerm(NLDB0::getAssignInput())->setNet(scalar);
      assign1->getInstTerm(NLDB0::getAssignOutput())->setNet(bus->getBit(-1));
      assign2->getInstTerm(NLDB0::getAssignInput())->setNet(const1);
      assign2->getInstTerm(NLDB0::getAssignOutput())->setNet(bus->getBit(0));
      assign3->getInstTerm(NLDB0::getAssignInput())->setNet(scalar);
      assign3->getInstTerm(NLDB0::getAssignOutput())->setNet(bus->getBit(1));
      assign4->getInstTerm(NLDB0::getAssignInput())->setNet(const0);
      assign4->getInstTerm(NLDB0::getAssignOutput())->setNet(bus->getBit(2));

      SNLDesign* topBusAssignCompression = SNLDesign::create(library, NLName("top_bus_assign_compression"));
      auto sourceBus = SNLBusNet::create(topBusAssignCompression, 3, 0, NLName("source_bus"));
      auto sinkBus = SNLBusNet::create(topBusAssignCompression, 3, 0, NLName("sink_bus"));
      for (int bit = 3; bit >= 0; --bit) {
        auto assign = SNLInstance::create(topBusAssignCompression, NLDB0::getAssign());
        assign->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBus->getBit(bit));
        assign->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBus->getBit(bit));
      }

      SNLDesign* topBusAssignNonContiguous = SNLDesign::create(library, NLName("top_bus_assign_non_contiguous"));
      auto sourceBusNonContiguous = SNLBusNet::create(topBusAssignNonContiguous, 5, 0, NLName("source_bus"));
      auto sinkBusNonContiguous = SNLBusNet::create(topBusAssignNonContiguous, 5, 0, NLName("sink_bus"));
      for (auto bit: {5, 4, 2, 1, 0}) {
        auto assign = SNLInstance::create(topBusAssignNonContiguous, NLDB0::getAssign());
        assign->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusNonContiguous->getBit(bit));
        assign->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusNonContiguous->getBit(bit));
      }

      SNLDesign* topBusAssignReversed = SNLDesign::create(library, NLName("top_bus_assign_reversed"));
      auto sourceBusReversed = SNLBusNet::create(topBusAssignReversed, 3, 0, NLName("source_bus"));
      auto sinkBusReversed = SNLBusNet::create(topBusAssignReversed, 3, 0, NLName("sink_bus"));
      for (int outputBit = 3; outputBit >= 0; --outputBit) {
        auto assign = SNLInstance::create(topBusAssignReversed, NLDB0::getAssign());
        auto inputBit = 3-outputBit;
        assign->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusReversed->getBit(inputBit));
        assign->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusReversed->getBit(outputBit));
      }

      SNLDesign* topBusAssignShuffled = SNLDesign::create(library, NLName("top_bus_assign_shuffled"));
      auto sourceBusShuffled = SNLBusNet::create(topBusAssignShuffled, 3, 0, NLName("source_bus"));
      auto sinkBusShuffled = SNLBusNet::create(topBusAssignShuffled, 3, 0, NLName("sink_bus"));
      for (auto outputBit: {3, 1, 2, 0}) {
        auto assign = SNLInstance::create(topBusAssignShuffled, NLDB0::getAssign());
        assign->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusShuffled->getBit(outputBit));
        assign->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusShuffled->getBit(outputBit));
      }

      SNLDesign* topConstAssignCompression = SNLDesign::create(library, NLName("top_const_assign_compression"));
      auto const0Compression = SNLScalarNet::create(topConstAssignCompression);
      const0Compression->setType(SNLNet::Type::Assign0);
      auto const1Compression = SNLScalarNet::create(topConstAssignCompression);
      const1Compression->setType(SNLNet::Type::Assign1);
      auto sinkBusConstCompression = SNLBusNet::create(topConstAssignCompression, 3, 0, NLName("sink_bus"));
      for (int outputBit = 3; outputBit >= 0; --outputBit) {
        auto assign = SNLInstance::create(topConstAssignCompression, NLDB0::getAssign());
        auto inputNet = (outputBit % 2 == 1) ? const1Compression : const0Compression;
        assign->getInstTerm(NLDB0::getAssignInput())->setNet(inputNet);
        assign->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusConstCompression->getBit(outputBit));
      }

      SNLDesign* topConstAssignNonContiguous = SNLDesign::create(library, NLName("top_const_assign_non_contiguous"));
      auto const0NonContiguous = SNLScalarNet::create(topConstAssignNonContiguous);
      const0NonContiguous->setType(SNLNet::Type::Assign0);
      auto const1NonContiguous = SNLScalarNet::create(topConstAssignNonContiguous);
      const1NonContiguous->setType(SNLNet::Type::Assign1);
      auto sinkBusConstNonContiguous = SNLBusNet::create(topConstAssignNonContiguous, 5, 0, NLName("sink_bus"));
      for (auto outputBit: {5, 4, 2, 1, 0}) {
        auto assign = SNLInstance::create(topConstAssignNonContiguous, NLDB0::getAssign());
        auto inputNet = (outputBit == 5 or outputBit == 2 or outputBit == 0) ? const1NonContiguous : const0NonContiguous;
        assign->getInstTerm(NLDB0::getAssignInput())->setNet(inputNet);
        assign->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusConstNonContiguous->getBit(outputBit));
      }

      SNLDesign* topConstAssignReversed = SNLDesign::create(library, NLName("top_const_assign_reversed"));
      auto const0Reversed = SNLScalarNet::create(topConstAssignReversed);
      const0Reversed->setType(SNLNet::Type::Assign0);
      auto const1Reversed = SNLScalarNet::create(topConstAssignReversed);
      const1Reversed->setType(SNLNet::Type::Assign1);
      auto sinkBusConstReversed = SNLBusNet::create(topConstAssignReversed, 3, 0, NLName("sink_bus"));
      for (int outputBit = 0; outputBit <= 3; ++outputBit) {
        auto assign = SNLInstance::create(topConstAssignReversed, NLDB0::getAssign());
        auto inputNet = (outputBit % 2 == 0) ? const1Reversed : const0Reversed;
        assign->getInstTerm(NLDB0::getAssignInput())->setNet(inputNet);
        assign->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusConstReversed->getBit(outputBit));
      }

      SNLDesign* topScalarAssignOutput = SNLDesign::create(library, NLName("top_scalar_assign_output"));
      auto sourceBusScalarOutput = SNLBusNet::create(topScalarAssignOutput, 1, 0, NLName("source_bus"));
      auto scalarSink = SNLScalarNet::create(topScalarAssignOutput, NLName("scalar_sink"));
      auto assignScalarOutput = SNLInstance::create(topScalarAssignOutput, NLDB0::getAssign());
      assignScalarOutput->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusScalarOutput->getBit(0));
      assignScalarOutput->getInstTerm(NLDB0::getAssignOutput())->setNet(scalarSink);

      SNLDesign* topAppendAssignScalarOutput = SNLDesign::create(library, NLName("top_append_assign_scalar_output"));
      auto sourceBusAppendScalarOutput = SNLBusNet::create(topAppendAssignScalarOutput, 1, 0, NLName("source_bus"));
      auto sinkBusAppendScalarOutput = SNLBusNet::create(topAppendAssignScalarOutput, 1, 0, NLName("sink_bus"));
      auto scalarSinkAppendScalarOutput = SNLScalarNet::create(topAppendAssignScalarOutput, NLName("scalar_sink"));
      auto appendAssign0 = SNLInstance::create(topAppendAssignScalarOutput, NLDB0::getAssign());
      appendAssign0->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusAppendScalarOutput->getBit(1));
      appendAssign0->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusAppendScalarOutput->getBit(1));
      auto appendAssign1 = SNLInstance::create(topAppendAssignScalarOutput, NLDB0::getAssign());
      appendAssign1->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusAppendScalarOutput->getBit(0));
      appendAssign1->getInstTerm(NLDB0::getAssignOutput())->setNet(scalarSinkAppendScalarOutput);

      SNLDesign* topAppendAssignDifferentOutputBus = SNLDesign::create(library, NLName("top_append_assign_different_output_bus"));
      auto sourceBusDifferentOutputBus = SNLBusNet::create(topAppendAssignDifferentOutputBus, 1, 0, NLName("source_bus"));
      auto sinkBusA = SNLBusNet::create(topAppendAssignDifferentOutputBus, 1, 0, NLName("sink_bus_a"));
      auto sinkBusB = SNLBusNet::create(topAppendAssignDifferentOutputBus, 1, 0, NLName("sink_bus_b"));
      auto appendDifferentOutputBusAssign0 = SNLInstance::create(topAppendAssignDifferentOutputBus, NLDB0::getAssign());
      appendDifferentOutputBusAssign0->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusDifferentOutputBus->getBit(1));
      appendDifferentOutputBusAssign0->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusA->getBit(1));
      auto appendDifferentOutputBusAssign1 = SNLInstance::create(topAppendAssignDifferentOutputBus, NLDB0::getAssign());
      appendDifferentOutputBusAssign1->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusDifferentOutputBus->getBit(0));
      appendDifferentOutputBusAssign1->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusB->getBit(0));

      SNLDesign* topAppendAssignDifferentInputBus = SNLDesign::create(library, NLName("top_append_assign_different_input_bus"));
      auto sourceBusAForDifferentInputBus = SNLBusNet::create(topAppendAssignDifferentInputBus, 1, 0, NLName("source_bus_a"));
      auto sourceBusBForDifferentInputBus = SNLBusNet::create(topAppendAssignDifferentInputBus, 1, 0, NLName("source_bus_b"));
      auto sinkBusForDifferentInputBus = SNLBusNet::create(topAppendAssignDifferentInputBus, 1, 0, NLName("sink_bus"));
      auto appendDifferentInputBusAssign0 = SNLInstance::create(topAppendAssignDifferentInputBus, NLDB0::getAssign());
      appendDifferentInputBusAssign0->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusAForDifferentInputBus->getBit(1));
      appendDifferentInputBusAssign0->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusForDifferentInputBus->getBit(1));
      auto appendDifferentInputBusAssign1 = SNLInstance::create(topAppendAssignDifferentInputBus, NLDB0::getAssign());
      appendDifferentInputBusAssign1->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusBForDifferentInputBus->getBit(0));
      appendDifferentInputBusAssign1->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusForDifferentInputBus->getBit(0));

      SNLDesign* topAppendAssignInputNonContiguous = SNLDesign::create(library, NLName("top_append_assign_input_non_contiguous"));
      auto sourceBusInputNonContiguous = SNLBusNet::create(topAppendAssignInputNonContiguous, 3, 0, NLName("source_bus"));
      auto sinkBusInputNonContiguous = SNLBusNet::create(topAppendAssignInputNonContiguous, 3, 0, NLName("sink_bus"));
      auto appendInputNonContiguousAssign0 = SNLInstance::create(topAppendAssignInputNonContiguous, NLDB0::getAssign());
      appendInputNonContiguousAssign0->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusInputNonContiguous->getBit(3));
      appendInputNonContiguousAssign0->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusInputNonContiguous->getBit(3));
      auto appendInputNonContiguousAssign1 = SNLInstance::create(topAppendAssignInputNonContiguous, NLDB0::getAssign());
      appendInputNonContiguousAssign1->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusInputNonContiguous->getBit(1));
      appendInputNonContiguousAssign1->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusInputNonContiguous->getBit(2));

      SNLDesign* topAppendAssignInputStepMismatch = SNLDesign::create(library, NLName("top_append_assign_input_step_mismatch"));
      auto sourceBusInputStepMismatch = SNLBusNet::create(topAppendAssignInputStepMismatch, 3, 0, NLName("source_bus"));
      auto sinkBusInputStepMismatch = SNLBusNet::create(topAppendAssignInputStepMismatch, 3, 0, NLName("sink_bus"));
      auto appendInputStepMismatchAssign0 = SNLInstance::create(topAppendAssignInputStepMismatch, NLDB0::getAssign());
      appendInputStepMismatchAssign0->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusInputStepMismatch->getBit(3));
      appendInputStepMismatchAssign0->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusInputStepMismatch->getBit(3));
      auto appendInputStepMismatchAssign1 = SNLInstance::create(topAppendAssignInputStepMismatch, NLDB0::getAssign());
      appendInputStepMismatchAssign1->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusInputStepMismatch->getBit(2));
      appendInputStepMismatchAssign1->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusInputStepMismatch->getBit(2));
      auto appendInputStepMismatchAssign2 = SNLInstance::create(topAppendAssignInputStepMismatch, NLDB0::getAssign());
      appendInputStepMismatchAssign2->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusInputStepMismatch->getBit(3));
      appendInputStepMismatchAssign2->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusInputStepMismatch->getBit(1));

      SNLDesign* topAppendAssignOutputStepMismatch = SNLDesign::create(library, NLName("top_append_assign_output_step_mismatch"));
      auto sourceBusOutputStepMismatch = SNLBusNet::create(topAppendAssignOutputStepMismatch, 3, 0, NLName("source_bus"));
      auto sinkBusOutputStepMismatch = SNLBusNet::create(topAppendAssignOutputStepMismatch, 3, 0, NLName("sink_bus"));
      auto outputStepMismatchAssign0 = SNLInstance::create(topAppendAssignOutputStepMismatch, NLDB0::getAssign());
      outputStepMismatchAssign0->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusOutputStepMismatch->getBit(3));
      outputStepMismatchAssign0->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusOutputStepMismatch->getBit(3));
      auto outputStepMismatchAssign1 = SNLInstance::create(topAppendAssignOutputStepMismatch, NLDB0::getAssign());
      outputStepMismatchAssign1->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusOutputStepMismatch->getBit(2));
      outputStepMismatchAssign1->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusOutputStepMismatch->getBit(2));
      auto outputStepMismatchAssign2 = SNLInstance::create(topAppendAssignOutputStepMismatch, NLDB0::getAssign());
      outputStepMismatchAssign2->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusOutputStepMismatch->getBit(1));
      outputStepMismatchAssign2->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusOutputStepMismatch->getBit(3));

      SNLDesign* topAppendAssignScalarInput = SNLDesign::create(library, NLName("top_append_assign_scalar_input"));
      auto sourceBusScalarInput = SNLBusNet::create(topAppendAssignScalarInput, 1, 0, NLName("source_bus"));
      auto sinkBusScalarInput = SNLBusNet::create(topAppendAssignScalarInput, 1, 0, NLName("sink_bus"));
      auto scalarInput = SNLScalarNet::create(topAppendAssignScalarInput, NLName("scalar_in"));
      auto appendScalarInputAssign0 = SNLInstance::create(topAppendAssignScalarInput, NLDB0::getAssign());
      appendScalarInputAssign0->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusScalarInput->getBit(1));
      appendScalarInputAssign0->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusScalarInput->getBit(1));
      auto appendScalarInputAssign1 = SNLInstance::create(topAppendAssignScalarInput, NLDB0::getAssign());
      appendScalarInputAssign1->getInstTerm(NLDB0::getAssignInput())->setNet(scalarInput);
      appendScalarInputAssign1->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusScalarInput->getBit(0));

      SNLDesign* topAssignsBeforeGate = SNLDesign::create(library, NLName("top_assigns_before_gate"));
      auto sourceBusBeforeGate = SNLBusNet::create(topAssignsBeforeGate, 1, 0, NLName("source_bus"));
      auto sinkBusBeforeGate = SNLBusNet::create(topAssignsBeforeGate, 1, 0, NLName("sink_bus"));
      auto gateOutBeforeGate = SNLScalarNet::create(topAssignsBeforeGate, NLName("gate_out"));
      for (int bit = 1; bit >= 0; --bit) {
        auto assign = SNLInstance::create(topAssignsBeforeGate, NLDB0::getAssign());
        assign->getInstTerm(NLDB0::getAssignInput())->setNet(sourceBusBeforeGate->getBit(bit));
        assign->getInstTerm(NLDB0::getAssignOutput())->setNet(sinkBusBeforeGate->getBit(bit));
      }
      auto and2 = SNLInstance::create(
        topAssignsBeforeGate,
        NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2),
        NLName("and2"));
      and2->getInstTerm(NLDB0::getGateSingleTerm(and2->getModel()))->setNet(gateOutBeforeGate);
      auto and2Inputs = NLDB0::getGateNTerms(and2->getModel());
      and2->getInstTerm(and2Inputs->getBitAtPosition(0))->setNet(sinkBusBeforeGate->getBit(0));
      and2->getInstTerm(and2Inputs->getBitAtPosition(1))->setNet(sinkBusBeforeGate->getBit(1));

      SNLDesign* unnamedInstanceModel = SNLDesign::create(library, NLName("unnamed_instance_model"));
      auto modelInput = SNLScalarTerm::create(unnamedInstanceModel, SNLTerm::Direction::Input, NLName("i"));
      auto modelOutput = SNLScalarTerm::create(unnamedInstanceModel, SNLTerm::Direction::Output, NLName("o"));
      SNLDesign* topUnnamedInstanceName = SNLDesign::create(library, NLName("top_unnamed_instance_name"));
      auto inputNet = SNLScalarNet::create(topUnnamedInstanceName, NLName("input_net"));
      auto outputNet = SNLScalarNet::create(topUnnamedInstanceName, NLName("output_net"));
      auto unnamedInstance = SNLInstance::create(topUnnamedInstanceName, unnamedInstanceModel);
      unnamedInstance->getInstTerm(modelInput)->setNet(inputNet);
      unnamedInstance->getInstTerm(modelOutput)->setNet(outputNet);

    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    NLDB*      db_;
};

TEST_F(SNLVRLDumperTest2, test0) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2Test0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2Test0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testUnconnectedAssigns) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top"));
  ASSERT_TRUE(top);

  //Destroy net: scalar
  auto scalar = top->getScalarNet(NLName("scalar"));
  ASSERT_TRUE(scalar);
  scalar->destroy();

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestUnconnectedAssigns";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestUnconnectedAssigns" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testBusAssignCompression) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_bus_assign_compression"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestBusAssignCompression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestBusAssignCompression" / "top_bus_assign_compression.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testBusAssignNonContiguous) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_bus_assign_non_contiguous"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestBusAssignNonContiguous";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestBusAssignNonContiguous" / "top_bus_assign_non_contiguous.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testBusAssignReversed) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_bus_assign_reversed"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestBusAssignReversed";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestBusAssignReversed" / "top_bus_assign_reversed.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testBusAssignShuffledOrder) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_bus_assign_shuffled"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestBusAssignShuffledOrder";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestBusAssignShuffledOrder" / "top_bus_assign_shuffled.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testConstAssignCompression) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_const_assign_compression"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestConstAssignCompression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestConstAssignCompression" / "top_const_assign_compression.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testConstAssignNonContiguous) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_const_assign_non_contiguous"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestConstAssignNonContiguous";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestConstAssignNonContiguous" / "top_const_assign_non_contiguous.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testConstAssignReversed) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_const_assign_reversed"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestConstAssignReversed";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestConstAssignReversed" / "top_const_assign_reversed.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testScalarAssignOutput) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_scalar_assign_output"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestScalarAssignOutput";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestScalarAssignOutput" / "top_scalar_assign_output.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testAppendAssignScalarOutput) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_append_assign_scalar_output"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestAppendAssignScalarOutput";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestAppendAssignScalarOutput" / "top_append_assign_scalar_output.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testAppendAssignDifferentOutputBus) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_append_assign_different_output_bus"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestAppendAssignDifferentOutputBus";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestAppendAssignDifferentOutputBus" / "top_append_assign_different_output_bus.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testAppendAssignDifferentInputBus) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_append_assign_different_input_bus"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestAppendAssignDifferentInputBus";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestAppendAssignDifferentInputBus" / "top_append_assign_different_input_bus.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testAppendAssignInputNonContiguous) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_append_assign_input_non_contiguous"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestAppendAssignInputNonContiguous";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestAppendAssignInputNonContiguous" / "top_append_assign_input_non_contiguous.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testAppendAssignInputStepMismatch) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_append_assign_input_step_mismatch"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestAppendAssignInputStepMismatch";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestAppendAssignInputStepMismatch" / "top_append_assign_input_step_mismatch.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testAppendAssignOutputStepMismatch) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_append_assign_output_step_mismatch"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestAppendAssignOutputStepMismatch";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestAppendAssignOutputStepMismatch" / "top_append_assign_output_step_mismatch.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testAppendAssignScalarInput) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_append_assign_scalar_input"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestAppendAssignScalarInput";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestAppendAssignScalarInput" / "top_append_assign_scalar_input.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testAssignsBeforeGate) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_assigns_before_gate"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestAssignsBeforeGate";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestAssignsBeforeGate" / "top_assigns_before_gate.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testUnnamedInstanceName) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top_unnamed_instance_name"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestUnnamedInstanceName";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestUnnamedInstanceName" / "top_unnamed_instance_name.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}
