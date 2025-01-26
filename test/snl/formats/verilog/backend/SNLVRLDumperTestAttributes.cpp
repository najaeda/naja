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

using namespace naja::SNL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif
#ifndef NAJA_DIFF
#define NAJA_DIFF "Undefined"
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

  SNLAttributes::addAttribute(top_,
    SNLAttribute(SNLName("PRAGMA1"), SNLAttributeValue("value1")));
  SNLAttributes::addAttribute(top_,
    SNLAttribute(
      SNLName("PRAGMA2"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "12")));
  SNLAttributes::addAttribute(top_, SNLAttribute(SNLName("PRAGMA2")));

  auto term = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, SNLName("term"));
  SNLAttributes::addAttribute(term,
    SNLAttribute(SNLName("TPRAGMA1"), SNLAttributeValue("value1")));
  SNLAttributes::addAttribute(term,
    SNLAttribute(
      SNLName("TPRAGMA2"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "155")));
  SNLAttributes::addAttribute(term, SNLAttribute(SNLName("TPRAGMA2")));

  auto net = SNLScalarNet::create(top_, SNLName("net"));
  SNLAttributes::addAttribute(net,
    SNLAttribute(SNLName("NPRAGMA1"), SNLAttributeValue("value1")));
  SNLAttributes::addAttribute(net,
    SNLAttribute(
      SNLName("NPRAGMA2"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "88")));
  SNLAttributes::addAttribute(net, SNLAttribute(SNLName("NPRAGMA2")));

  auto instance = SNLInstance::create(top_, model_, SNLName("ins"));
  SNLAttributes::addAttribute(instance,
    SNLAttribute(SNLName("IPRAGMA1"), SNLAttributeValue("value1")));
  SNLAttributes::addAttribute(instance,
    SNLAttribute(
      SNLName("IPRAGMA2"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "9")));
  SNLAttributes::addAttribute(instance, SNLAttribute(SNLName("IPRAGMA2")));

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

  outPath = outPath / (top_->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testAttributes0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}
