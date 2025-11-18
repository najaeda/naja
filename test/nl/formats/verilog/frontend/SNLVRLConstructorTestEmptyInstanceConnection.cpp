// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

#include "NLUniverse.h"

#include "SNLVRLConstructor.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestEmptyInstanceConnection: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTestEmptyInstanceConnection, testKnownModel) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"test_empty_instance_connection0.v");
  auto test0 = library_->getSNLDesign(NLName("test_empty_instance_connection"));

  using Instances = std::vector<SNLInstance*>;
  Instances instances(test0->getInstances().begin(), test0->getInstances().end());
  ASSERT_EQ(1, instances.size());
  auto ins0 = instances[0];
  ASSERT_NE(ins0, nullptr);
  EXPECT_FALSE(ins0->isUnnamed());

  auto model = ins0->getModel();
  ASSERT_NE(model, nullptr);
  EXPECT_TRUE(model->isBlackBox());
  EXPECT_EQ(1, model->getTerms().size());
  
  auto aTerm = model->getTerm(NLName("A"));
  ASSERT_NE(aTerm, nullptr);
  EXPECT_EQ(aTerm->getWidth(), 1);
  EXPECT_EQ(aTerm->getBits().size(), 1);
  EXPECT_EQ(SNLTerm::Direction::Input, aTerm->getDirection());

  auto inst0 = instances[0];
  ASSERT_NE(inst0, nullptr);
  EXPECT_FALSE(inst0->isUnnamed());

  EXPECT_EQ(model, inst0->getModel());
}

TEST_F(SNLVRLConstructorTestEmptyInstanceConnection, testUnknownModel) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  constructor.config_.allowUnknownDesigns_ = true;
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"test_empty_instance_connection1.v");
  auto test0 = library_->getSNLDesign(NLName("test_empty_instance_connection"));

  using Instances = std::vector<SNLInstance*>;
  Instances instances(test0->getInstances().begin(), test0->getInstances().end());
  ASSERT_EQ(1, instances.size());
  auto ins0 = instances[0];
  ASSERT_NE(ins0, nullptr);
  EXPECT_FALSE(ins0->isUnnamed());

  auto model = ins0->getModel();
  ASSERT_NE(model, nullptr);
  EXPECT_TRUE(model->isAutoBlackBox());
  EXPECT_TRUE(model->isBlackBox());
  EXPECT_EQ(1, model->getTerms().size());
  
  auto aTerm = model->getTerm(NLName("A"));
  ASSERT_NE(aTerm, nullptr);
  EXPECT_EQ(aTerm->getWidth(), 1);
  EXPECT_EQ(aTerm->getBits().size(), 1);
  EXPECT_EQ(SNLTerm::Direction::Undefined, aTerm->getDirection());

  auto inst0 = instances[0];
  ASSERT_NE(inst0, nullptr);
  EXPECT_FALSE(inst0->isUnnamed());

  EXPECT_EQ(model, inst0->getModel());
}