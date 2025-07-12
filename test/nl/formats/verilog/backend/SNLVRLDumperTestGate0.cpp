// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

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
      and5->getInstTerm(NLDB0::getNInputGateOutput(and5->getModel()))->setNet(out0Net);
      auto inputs = NLDB0::getNInputGateInputs(and5->getModel());
      for (size_t i = 0; i < inputs->getWidth(); ++i) {
        auto instTerm = and5->getInstTerm(inputs->getBitAtPosition(i));
        instTerm->setNet(busNet->getBit(i));
      }

      //anonymous gate instance
      auto xor5 =
        SNLInstance::create(design, NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 5));
      xor5->getInstTerm(NLDB0::getNInputGateOutput(xor5->getModel()))->setNet(out1Net);
      inputs = NLDB0::getNInputGateInputs(xor5->getModel());
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