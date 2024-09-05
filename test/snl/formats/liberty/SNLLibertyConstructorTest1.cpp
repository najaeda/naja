// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

#include "SNLLibertyConstructor.h"

using namespace naja::SNL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    SNLLibrary*      library_;
};

TEST_F(SNLLibertyConstructorTest1, testBusses) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bus_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("bus_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("ram"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(4, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  EXPECT_EQ(3, design->getBusTerms().size());
  auto clk = design->getScalarTerm(SNLName("clk"));
  ASSERT_NE(nullptr, clk);
  EXPECT_EQ(SNLTerm::Direction::Input, clk->getDirection());
  auto rd_out = design->getBusTerm(SNLName("rd_out"));
  ASSERT_NE(nullptr, rd_out);
  EXPECT_EQ(SNLTerm::Direction::Output, rd_out->getDirection());
  EXPECT_EQ(14, rd_out->getMSB());
  EXPECT_EQ(0, rd_out->getLSB());
}

TEST_F(SNLLibertyConstructorTest1, testBufferFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("buffer_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("buffer_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("buffer"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(SNLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
}

TEST_F(SNLLibertyConstructorTest1, testBufZFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bufz.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("bufz_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("bufz"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(SNLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto en = design->getScalarTerm(SNLName("EN"));
  ASSERT_NE(nullptr, en);
  EXPECT_EQ(SNLTerm::Direction::Input, en->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
}