// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "NLUniverse.h"
#include "NLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
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

class SNLVRLDumperTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
      SNLDesign* design = SNLDesign::create(library, NLName("design"));

      SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("i0"));
      SNLBusTerm::create(design, SNLTerm::Direction::Input, 31, 0, NLName("i1"));
      SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("o"));

      SNLScalarNet::create(design);
      SNLBusNet::create(design, 31, 0);
      SNLScalarNet::create(design, NLName("n1"));
      SNLScalarNet::create(design, NLName("n2"));

      SNLDesign* model = SNLDesign::create(library, NLName("model"));
      SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
      SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("o"));
      SNLScalarTerm::create(model, SNLTerm::Direction::InOut, NLName("io"));
      SNLInstance* instance1 = SNLInstance::create(design, model, NLName("instance1"));
      SNLInstance* instance2 = SNLInstance::create(design, model, NLName("instance2"));

      //connections between instances
      instance1->getInstTerm(model->getScalarTerm(NLName("o")))->setNet(design->getScalarNet(NLName("n1")));
      instance2->getInstTerm(model->getScalarTerm(NLName("i")))->setNet(design->getScalarNet(NLName("n1")));
      instance1->getInstTerm(model->getScalarTerm(NLName("io")))->setNet(design->getScalarNet(NLName("n2")));
      instance2->getInstTerm(model->getScalarTerm(NLName("io")))->setNet(design->getScalarNet(NLName("n2")));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    NLDB* db_;
};

TEST_F(SNLVRLDumperTest0, testNonExistingPath) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("design"));
  ASSERT_TRUE(top);
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "ERROR";
  SNLVRLDumper dumper;
  ASSERT_THROW(dumper.dumpDesign(top, outPath), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTest0, testSingleFile) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("design"));
  ASSERT_TRUE(top);
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test0SingleFile";
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
  referencePath = referencePath / "test0SingleFile" / "design.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest0, testMultipleFiles) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getSNLDesign(NLName("design"));
  ASSERT_TRUE(top);
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test0MultipleFiles";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setSingleFile(false);
  dumper.dumpDesign(top, outPath);
}
