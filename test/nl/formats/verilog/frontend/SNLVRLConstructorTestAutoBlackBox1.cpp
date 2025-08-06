// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestAutoBlackBox1: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("MYLIB"));
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
      library_ = nullptr;
    }
  protected:
    NLLibrary*      library_;
};

TEST_F(SNLVRLConstructorTestAutoBlackBox1, test0) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  constructor.config_.allowUnknownDesigns_ = true;
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"auto_blackbox_test1.v");
  auto test0 = library_->getSNLDesign(NLName("test0"));

  using Instances = std::vector<SNLInstance*>;
  Instances instances(test0->getInstances().begin(), test0->getInstances().end());
  ASSERT_EQ(2, instances.size());
  auto ins0 = instances[0];
  ASSERT_NE(ins0, nullptr);
  EXPECT_FALSE(ins0->isUnnamed());

  auto model = ins0->getModel();
  ASSERT_NE(model, nullptr);
  EXPECT_TRUE(model->isAutoBlackBox());
  EXPECT_TRUE(model->isBlackBox());
  EXPECT_EQ(5, model->getTerms().size());
  
  auto aTerm = model->getTerm(NLName("A"));
  ASSERT_NE(aTerm, nullptr);
  EXPECT_EQ(aTerm->getWidth(), 1);
  EXPECT_EQ(aTerm->getBits().size(), 1);
  EXPECT_EQ(SNLTerm::Direction::Input, aTerm->getDirection());

  auto ins1 = instances[0];
  ASSERT_NE(ins1, nullptr);
  EXPECT_FALSE(ins1->isUnnamed());

  EXPECT_EQ(model, ins1->getModel());

}

TEST_F(SNLVRLConstructorTestAutoBlackBox1, test0LoadAndDump) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  constructor.config_.allowUnknownDesigns_ = true;
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"auto_blackbox_test1.v");
  auto test0 = library_->getSNLDesign(NLName("test0"));
  EXPECT_NE(nullptr, test0);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "auto_blackbox_test1";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName("test0.v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(test0, outPath);
}