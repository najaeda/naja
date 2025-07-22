// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

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

class SNLVRLConstructorTestGate2: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTestGate2, test) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test_gates2.v");

  ASSERT_EQ(1, library_->getSNLDesigns().size());
  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  ASSERT_EQ(3, top->getTerms().size());
  using Terms = std::vector<SNLTerm*>;
  Terms terms(top->getTerms().begin(), top->getTerms().end());
  EXPECT_THAT(terms,
    ElementsAre(
      top->getTerm(NLName("n0")),
      top->getTerm(NLName("n1")),
      top->getTerm(NLName("n2"))
    )
  );

  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test_gates2.v");
  auto findTop = SNLUtils::findTop(library_);
  EXPECT_EQ(findTop, top);

  using Instances = std::vector<SNLInstance*>;
  Instances instances(top->getInstances().begin(), top->getInstances().end());
  ASSERT_EQ(3, instances.size());

  EXPECT_THAT(instances,
    ElementsAre(
      top->getInstance(NLName("g0")),
      top->getInstance(NLName("g1")),
      top->getInstance(NLName("g2"))
    )
  );

  auto g0 = instances[0];
  ASSERT_NE(g0, nullptr);
  EXPECT_FALSE(g0->isUnnamed());
  EXPECT_EQ("g0", g0->getName().getString());

  auto g0Model = g0->getModel(); // go is a nand
  EXPECT_TRUE(NLDB0::isNInputGate(g0Model));
  auto g0SingleTerm = NLDB0::getGateSingleTerm(g0Model);
  EXPECT_NE(g0SingleTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Output, g0SingleTerm->getDirection());
  auto g0MultiTerm = NLDB0::getGateNTerms(g0Model);
  EXPECT_EQ(SNLTerm::Direction::Input, g0MultiTerm->getDirection());
  EXPECT_EQ(g0MultiTerm->getWidth(), 2);
  EXPECT_EQ(g0MultiTerm->getBits().size(), 2);

  auto g1 = instances[1];
  ASSERT_NE(g1, nullptr);
  EXPECT_FALSE(g1->isUnnamed());
  EXPECT_EQ("g1", g1->getName().getString());
  auto g1Model = g1->getModel(); // g1 is a not
  EXPECT_TRUE(NLDB0::isNOutputGate(g1Model));
  auto g1SingleTerm = NLDB0::getGateSingleTerm(g1Model);
  EXPECT_NE(g1SingleTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Input, g1SingleTerm->getDirection());
  auto g1MultiTerm = NLDB0::getGateNTerms(g1Model);
  EXPECT_EQ(SNLTerm::Direction::Output, g1MultiTerm->getDirection());
  EXPECT_EQ(g1MultiTerm->getWidth(), 1);
  EXPECT_EQ(g1MultiTerm->getBits().size(), 1);
}

TEST_F(SNLVRLConstructorTestGate2, testLoadAndDump) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test_gates2.v");

  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test_gates2.v");
  auto top = SNLUtils::findTop(library_);
  EXPECT_NE(nullptr, top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test_gates2";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);
}