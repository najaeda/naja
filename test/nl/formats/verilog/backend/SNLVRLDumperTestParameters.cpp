// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "NLUniverse.h"
#include "NLDB.h"
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

class SNLVRLDumperTestParameters: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      NLDB* db = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
      model_ = SNLDesign::create(library, NLName("model"));
      top_ = SNLDesign::create(library, NLName("top"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    SNLDesign*  top_;
    SNLDesign*  model_;
};

TEST_F(SNLVRLDumperTestParameters, test0) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);
  SNLParameter::create(model_, NLName("PARAM0"), SNLParameter::Type::Decimal, "8");
  auto falseParam = SNLParameter::create(model_, NLName("PARAM1"), SNLParameter::Type::Boolean, "0");
  auto trueParam = SNLParameter::create(model_, NLName("PARAM2"), SNLParameter::Type::Boolean, "1");
  SNLParameter::create(model_, NLName("PARAM3"), SNLParameter::Type::Binary, "4'hF");
  SNLParameter::create(model_, NLName("PARAM4"), SNLParameter::Type::Binary, "4'b0011");
  SNLParameter::create(model_, NLName("PARAM5"), SNLParameter::Type::String, "HELLO");
  auto ins = SNLInstance::create(top_, model_, NLName("ins"));
  SNLInstParameter::create(ins, falseParam, "1");
  SNLInstParameter::create(ins, trueParam, "0");

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParameters0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  outPath = outPath / (top_->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testParameters0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestParameters, testErrors0) {
  ASSERT_TRUE(top_);
  SNLParameter::create(top_, NLName("PARAM"), SNLParameter::Type::Boolean, "YY");
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParametersErrors0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTestParameters, testErrors1) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);
  auto falseParam = SNLParameter::create(model_, NLName("PARAM1"), SNLParameter::Type::Boolean, "0");
  auto trueParam = SNLParameter::create(model_, NLName("PARAM2"), SNLParameter::Type::Boolean, "1");
  auto ins = SNLInstance::create(top_, model_, NLName("ins"));
  SNLInstParameter::create(ins, falseParam, "Y");
  SNLInstParameter::create(ins, trueParam, "N");

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParametersErrors1";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
}
