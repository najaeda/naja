// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

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
class SNLVRLDumperTestEscaping: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
      SNLDesign* model0 = SNLDesign::create(library, NLName("#model0"));
      auto t0 = SNLScalarTerm::create(model0, SNLTerm::Direction::Input, NLName("%t0"));
      auto t1 = SNLScalarTerm::create(model0, SNLTerm::Direction::Input, NLName("12t1@"));
      auto t2 = SNLBusTerm::create(model0, SNLTerm::Direction::Input, 3, 0, NLName("3 4"));
      auto t3 = SNLBusTerm::create(model0, SNLTerm::Direction::Input, -5, 2, NLName("##"));
      auto t4 = SNLScalarTerm::create(model0, SNLTerm::Direction::Input, NLName("___$$"));
      SNLDesign* top = SNLDesign::create(library, NLName("design@"));
      universe->setTopDesign(top);

      auto ins = SNLInstance::create(top, model0, NLName("0ins"));
      auto n0 = SNLScalarNet::create(top, NLName("^n0^"));
      auto n1 = SNLScalarNet::create(top, NLName("[n1]"));
      auto n2 = SNLBusNet::create(top, 3, 0, NLName("3 4"));
      auto n3 = SNLBusNet::create(top, -5, 2, NLName("##"));
      auto n4 = SNLScalarNet::create(top, NLName("_$$__"));
      ins->setTermNet(t0, n0);
      ins->setTermNet(t1, n1);
      ins->setTermNet(t2, n2);
      ins->setTermNet(t3, n3);
      ins->setTermNet(t4, n4);
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    NLDB*      db_;
};

TEST_F(SNLVRLDumperTestEscaping, test) {
  auto top = NLUniverse::get()->getTopDesign();
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testEscaping";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  auto fileName = top->getName().getString() + ".v"; 
  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testEscaping" / fileName;
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}
