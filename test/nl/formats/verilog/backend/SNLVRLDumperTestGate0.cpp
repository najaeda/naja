// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "SNLVRLDumper.h"

#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarNet.h"

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

//Test gate designs
class SNLVRLDumperTestGate0: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
      SNLDesign* design = SNLDesign::create(library, NLName("top"));

      auto busNet = SNLBusNet::create(design, 4, 0, NLName("busNet"));
      auto out0Net = SNLScalarNet::create(design, NLName("out0Net"));
      auto out1Net = SNLScalarNet::create(design, NLName("out1Net"));

      auto and5 =
        SNLInstance::create(design, NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 5), NLName("and5"));
      and5->getInstTerm(NLDB0::getGateSingleTerm(and5->getModel()))->setNet(out0Net);
      auto inputs = NLDB0::getGateNTerms(and5->getModel());
      for (size_t i = 0; i < inputs->getWidth(); ++i) {
        auto instTerm = and5->getInstTerm(inputs->getBitAtPosition(i));
        instTerm->setNet(busNet->getBit(i));
      }

      //anonymous gate instance
      auto xor5 =
        SNLInstance::create(design, NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 5));
      xor5->getInstTerm(NLDB0::getGateSingleTerm(xor5->getModel()))->setNet(out1Net);
      inputs = NLDB0::getGateNTerms(xor5->getModel());
      for (size_t i = 0; i < inputs->getWidth(); ++i) {
        auto instTerm = xor5->getInstTerm(inputs->getBitAtPosition(i));
        instTerm->setNet(busNet->getBit(i));
      }
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    NLDB* db_;
};

TEST_F(SNLVRLDumperTestGate0, test0) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test_gate0";
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
  referencePath = referencePath / "test_gate0" / "test_gate0.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestGate0, testGatePinsWithDummyAndAssignConstants) {
  auto lib = db_->getLibrary(NLName("MYLIB"));
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("top"));
  ASSERT_TRUE(top);

  auto assign0 = SNLScalarNet::create(top, NLName("assign0"));
  assign0->setType(SNLNet::Type::Assign0);
  auto assign1 = SNLScalarNet::create(top, NLName("assign1"));
  assign1->setType(SNLNet::Type::Assign1);
  auto gateOut = SNLScalarNet::create(top, NLName("mixedOut"));

  auto and3 = SNLInstance::create(
    top, NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 3), NLName("and3mix"));
  auto output = and3->getInstTerm(NLDB0::getGateSingleTerm(and3->getModel()));
  ASSERT_TRUE(output);
  output->setNet(gateOut);

  auto inputs = NLDB0::getGateNTerms(and3->getModel());
  ASSERT_TRUE(inputs);
  auto input0 = and3->getInstTerm(inputs->getBitAtPosition(0));
  auto input1 = and3->getInstTerm(inputs->getBitAtPosition(1));
  auto input2 = and3->getInstTerm(inputs->getBitAtPosition(2));
  ASSERT_TRUE(input0);
  ASSERT_TRUE(input1);
  ASSERT_TRUE(input2);
  input0->setNet(assign0);
  input1->setNet(assign1);
  EXPECT_EQ(nullptr, input2->getNet());

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test_gate0_constants_dummy";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  std::ifstream dumped(outPath / "top.v");
  ASSERT_TRUE(dumped.good());
  std::stringstream buffer;
  buffer << dumped.rdbuf();
  const auto content = buffer.str();

  EXPECT_NE(content.find("and and3mix("), std::string::npos);
  EXPECT_NE(content.find("1'b0"), std::string::npos);
  EXPECT_NE(content.find("1'b1"), std::string::npos);
  EXPECT_NE(content.find("DUMMY"), std::string::npos);
}
