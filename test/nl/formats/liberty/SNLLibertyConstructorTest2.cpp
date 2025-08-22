// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

#include "SNLLibertyConstructor.h"
#include "NLBitVecDynamic.h"
#include "SNLDesignModeling.h"
#include "SNLCapnP.h"

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest2: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      library_ = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*  library_;
    NLDB* db_ = nullptr;
};

TEST_F(SNLLibertyConstructorTest2, test) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("small.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("small_lib"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto and2 = library_->getSNLDesign(NLName("and2"));
  ASSERT_NE(nullptr, and2);
  EXPECT_EQ(3, and2->getTerms().size());
  EXPECT_EQ(3, and2->getScalarTerms().size());
  EXPECT_TRUE(and2->getBusTerms().empty());
  auto i0 = and2->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = and2->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = and2->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(and2);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(NLBitVecDynamic(0b1000, 4), tt.bits());
  //
  //EXPECT_TRUE(SNLDesignModeling::isAnd(design));

  auto ff = library_->getSNLDesign(NLName("FF"));
  ASSERT_NE(nullptr, ff);
  EXPECT_EQ(3, ff->getTerms().size());
  EXPECT_EQ(3, ff->getScalarTerms().size());
  EXPECT_TRUE(ff->getBusTerms().empty());
  auto d = ff->getScalarTerm(NLName("D"));
  ASSERT_NE(nullptr, d);
  EXPECT_EQ(SNLTerm::Direction::Input, d->getDirection());
  auto ck = ff->getScalarTerm(NLName("CK"));
  ASSERT_NE(nullptr, ck);
  EXPECT_EQ(SNLTerm::Direction::Input, ck->getDirection());
  auto q = ff->getScalarTerm(NLName("Q"));
  ASSERT_NE(nullptr, q);
  EXPECT_EQ(SNLTerm::Direction::Output, q->getDirection());
  auto ff_tt = SNLDesignModeling::getTruthTable(ff);
  EXPECT_FALSE(ff_tt.isInitialized());
  {
    auto clocks = SNLDesignModeling::getInputRelatedClocks(d);
    EXPECT_EQ(1, clocks.size());
  }
  {
    auto clocks = SNLDesignModeling::getOutputRelatedClocks(q);
    EXPECT_EQ(1, clocks.size());
  }
  // dump to naja if
  std::filesystem::path outPath("dump_if");
  SNLCapnP::dump(db_, outPath);
  {
    NLUniverse::get()->destroy();
    NLUniverse* universe = NLUniverse::create();
    db_ = NLDB::create(universe);
    library_ = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("MYLIB"));
    SNLLibertyConstructor constructor(library_);
    std::filesystem::path testPath(
        std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
        / std::filesystem::path("benchmarks")
        / std::filesystem::path("tests")
        / std::filesystem::path("small.lib"));
    constructor.construct(testPath);
  }
  auto db = SNLCapnP::load(outPath, true);
  NLUniverse::get()->destroy();
  EXPECT_THROW(SNLCapnP::load(outPath, true), NLException);
}