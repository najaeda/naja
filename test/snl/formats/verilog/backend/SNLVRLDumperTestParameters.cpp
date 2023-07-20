// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

using namespace naja::SNL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

class SNLVRLDumperTestParameters: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      SNLDB* db = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
      model_ = SNLDesign::create(library, SNLName("model"));
      top_ = SNLDesign::create(library, SNLName("top"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
  protected:
    SNLDesign*  top_;
    SNLDesign*  model_;
};

TEST_F(SNLVRLDumperTestParameters, test0) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);
  SNLParameter::create(model_, SNLName("PARAM0"), SNLParameter::Type::Decimal, "8");
  auto falseParam = SNLParameter::create(model_, SNLName("PARAM1"), SNLParameter::Type::Boolean, "0");
  auto trueParam = SNLParameter::create(model_, SNLName("PARAM2"), SNLParameter::Type::Boolean, "1");
  SNLParameter::create(model_, SNLName("PARAM3"), SNLParameter::Type::Binary, "4'hF");
  SNLParameter::create(model_, SNLName("PARAM4"), SNLParameter::Type::Binary, "4'b0011");
  SNLParameter::create(model_, SNLName("PARAM5"), SNLParameter::Type::String, "HELLO");
  auto ins = SNLInstance::create(top_, model_, SNLName("ins"));
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

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testParameters0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = "diff " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestParameters, testErrors0) {
  ASSERT_TRUE(top_);
  SNLParameter::create(top_, SNLName("PARAM"), SNLParameter::Type::Boolean, "YY");
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
  auto falseParam = SNLParameter::create(model_, SNLName("PARAM1"), SNLParameter::Type::Boolean, "0");
  auto trueParam = SNLParameter::create(model_, SNLName("PARAM2"), SNLParameter::Type::Boolean, "1");
  auto ins = SNLInstance::create(top_, model_, SNLName("ins"));
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