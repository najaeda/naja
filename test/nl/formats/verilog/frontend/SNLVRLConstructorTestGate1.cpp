// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;
//using ::testing::TypedEq;

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

class SNLVRLConstructorTestGate1: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTestGate1, test) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test_gates1.v");

  ASSERT_EQ(1, library_->getSNLDesigns().size());
  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  ASSERT_EQ(6, top->getTerms().size());
  using Terms = std::vector<SNLTerm*>;
  Terms terms(top->getTerms().begin(), top->getTerms().end());
  EXPECT_THAT(terms,
    ElementsAre(
      top->getTerm(NLName("A")),
      top->getTerm(NLName("B")),
      top->getTerm(NLName("C")),
      top->getTerm(NLName("D")),
      top->getTerm(NLName("E")),
      top->getTerm(NLName("OUT"))
    )
  );

  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test_gates1.v");
  auto findTop = SNLUtils::findTop(library_);
  EXPECT_EQ(findTop, top);

  using Instances = std::vector<SNLInstance*>;
  Instances instances(top->getInstances().begin(), top->getInstances().end());
  ASSERT_EQ(4, instances.size());
  auto ins0 = instances[0];
  ASSERT_NE(ins0, nullptr);
  EXPECT_TRUE(ins0->isAnonymous());
  auto nand3Model = ins0->getModel();
  EXPECT_TRUE(NLDB0::isNInputGate(nand3Model));
  auto nand3SingleTerm = NLDB0::getGateSingleTerm(nand3Model);
  EXPECT_NE(nand3SingleTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Output, nand3SingleTerm->getDirection());
  auto nand3MultiTerm = NLDB0::getGateNTerms(nand3Model);
  EXPECT_EQ(SNLTerm::Direction::Input, nand3MultiTerm->getDirection());
  EXPECT_EQ(nand3MultiTerm->getWidth(), 3);
  EXPECT_EQ(nand3MultiTerm->getBits().size(), 3);

  auto ins1 = instances[1];
  ASSERT_NE(ins1, nullptr);
  EXPECT_TRUE(ins1->isAnonymous());
  auto nor5Model = ins1->getModel();
  EXPECT_TRUE(NLDB0::isNInputGate(nor5Model));
  auto nor5SingleTerm = NLDB0::getGateSingleTerm(nor5Model);
  EXPECT_NE(nor5SingleTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Output, nor5SingleTerm->getDirection());
  auto nor5MultiTerm = NLDB0::getGateNTerms(nor5Model);
  EXPECT_EQ(SNLTerm::Direction::Input, nor5MultiTerm->getDirection());
  EXPECT_EQ(nor5MultiTerm->getWidth(), 5);
  EXPECT_EQ(nor5MultiTerm->getBits().size(), 5);

  auto ins2 = instances[2];
  ASSERT_NE(ins2, nullptr);
  EXPECT_TRUE(ins2->isAnonymous());
  auto xnor4Model = ins2->getModel();
  EXPECT_TRUE(NLDB0::isNInputGate(xnor4Model));

  auto ins3 = instances[3];
  ASSERT_NE(ins3, nullptr);
  EXPECT_TRUE(ins3->isAnonymous());
  auto nand3ModelBis = ins3->getModel();
  EXPECT_TRUE(NLDB0::isNInputGate(nand3ModelBis));
  EXPECT_EQ(nand3Model, nand3ModelBis); // should be the same as ins0
}

TEST_F(SNLVRLConstructorTestGate1, testLoadAndDump) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test_gates1.v");

  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test_gates1.v");
  auto top = SNLUtils::findTop(library_);
  EXPECT_NE(nullptr, top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test_gates1";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);
}