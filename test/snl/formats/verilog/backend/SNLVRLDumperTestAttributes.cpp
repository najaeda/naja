// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "SNLUniverse.h"
#include "SNLAttributes.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#if 0
#include "SNLDB.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#endif

using namespace naja::SNL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

class SNLVRLDumperTestAttributes: public ::testing::Test {
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

TEST_F(SNLVRLDumperTestAttributes, test0) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);

  SNLAttributes::addAttribute(top_, SNLAttributes::SNLAttribute(SNLName("PRAGMA1"), "value1"));
  SNLAttributes::addAttribute(top_, SNLAttributes::SNLAttribute(SNLName("PRAGMA2"), "value2"));
  SNLAttributes::addAttribute(top_, SNLAttributes::SNLAttribute(SNLName("PRAGMA2")));

  auto term = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, SNLName("term"));
  SNLAttributes::addAttribute(term, SNLAttributes::SNLAttribute(SNLName("TPRAGMA1"), "value1"));
  SNLAttributes::addAttribute(term, SNLAttributes::SNLAttribute(SNLName("TPRAGMA2"), "value2"));
  SNLAttributes::addAttribute(term, SNLAttributes::SNLAttribute(SNLName("TPRAGMA2")));

  auto net = SNLScalarNet::create(top_, SNLName("net"));
  SNLAttributes::addAttribute(net, SNLAttributes::SNLAttribute(SNLName("NPRAGMA1"), "value1"));
  SNLAttributes::addAttribute(net, SNLAttributes::SNLAttribute(SNLName("NPRAGMA2"), "value2"));
  SNLAttributes::addAttribute(net, SNLAttributes::SNLAttribute(SNLName("NPRAGMA2")));

  auto instance = SNLInstance::create(top_, model_, SNLName("ins"));
  SNLAttributes::addAttribute(instance, SNLAttributes::SNLAttribute(SNLName("IPRAGMA1"), "value1"));
  SNLAttributes::addAttribute(instance, SNLAttributes::SNLAttribute(SNLName("IPRAGMA2"), "value2"));
  SNLAttributes::addAttribute(instance, SNLAttributes::SNLAttribute(SNLName("IPRAGMA2")));

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testAttributes0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testAttributes0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = "diff " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

#if 0
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
#endif