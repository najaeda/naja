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

#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestGate0: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*      library_;
};

TEST_F(SNLVRLConstructorTestGate0, test) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test_gates0.v");

  ASSERT_EQ(1, library_->getSNLDesigns().size());
  auto fa = library_->getSNLDesign(NLName("FA"));
  ASSERT_NE(fa, nullptr);
  ASSERT_EQ(5, fa->getTerms().size());
  using Terms = std::vector<SNLTerm*>;
  Terms terms(fa->getTerms().begin(), fa->getTerms().end());
  EXPECT_THAT(terms,
    ElementsAre(
      fa->getTerm(NLName("A")),
      fa->getTerm(NLName("B")),
      fa->getTerm(NLName("C")),
      fa->getTerm(NLName("OUT")),
      fa->getTerm(NLName("COUT"))
    )
  );

  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test_gates0.v");
  auto top = SNLUtils::findTop(library_);
  EXPECT_EQ(top, fa);

  using Instances = std::vector<SNLInstance*>;
  Instances instances(top->getInstances().begin(), top->getInstances().end());
  ASSERT_EQ(9, instances.size());
  auto and0 = instances[0];
  ASSERT_NE(and0, nullptr);
  EXPECT_EQ("and0", and0->getName().getString());
  auto and2Model = and0->getModel();
  EXPECT_TRUE(NLDB0::isNInputGate(and2Model));

  auto not2 = instances[2];
  ASSERT_NE(not2, nullptr);
  EXPECT_EQ("not2", not2->getName().getString());
  auto not2Model = not2->getModel();
  EXPECT_TRUE(NLDB0::isNOutputGate(not2Model));
#if 0
  ASSERT_NE(lutModel, nullptr);
  EXPECT_EQ("LUT4", lutModel->getName().getString());
  ASSERT_EQ(1, lut->getInstParameters().size());
  auto initParam = *(lut->getInstParameters().begin());
  EXPECT_EQ("INIT", initParam->getName().getString());

  //
  auto inst2 = top->getInstance(NLName("inst2"));
  ASSERT_NE(nullptr, inst2);
  //verify instterms connectivity
  using InstTerms = std::vector<SNLInstTerm*>;
  InstTerms instTerms(inst2->getInstTerms().begin(), inst2->getInstTerms().end());
  ASSERT_EQ(3, instTerms.size());
  EXPECT_THAT(instTerms[0]->getBitTerm(), TypedEq<SNLTerm*>(model1->getTerm(NLName("i"))));
  EXPECT_THAT(instTerms[1]->getBitTerm(), TypedEq<SNLTerm*>(model1->getTerm(NLName("o"))));
  EXPECT_THAT(instTerms[2]->getBitTerm(), TypedEq<SNLTerm*>(model1->getTerm(NLName("io"))));
  EXPECT_THAT(instTerms[0]->getNet(), TypedEq<SNLNet*>(top->getNet(NLName("n2"))));
  EXPECT_THAT(instTerms[1]->getNet(), TypedEq<SNLNet*>(top->getNet(NLName("i"))));
  EXPECT_THAT(instTerms[2]->getNet(), TypedEq<SNLNet*>(top->getNet(NLName("io"))));
#endif
}

TEST_F(SNLVRLConstructorTestGate0, testErrors) {
  EXPECT_FALSE(NLDB0::isNInputGate(nullptr));
  EXPECT_FALSE(NLDB0::isNOutputGate(nullptr));
  EXPECT_FALSE(NLDB0::isGate(nullptr));

  auto model = SNLDesign::create(library_, NLName("testGate"));
  EXPECT_FALSE(NLDB0::isNInputGate(model));
  EXPECT_FALSE(NLDB0::isNOutputGate(model));
  EXPECT_FALSE(NLDB0::isGate(model));
  EXPECT_EQ(std::string(), NLDB0::getGateName(model));
}

TEST_F(SNLVRLConstructorTestGate0, testLoadAndDump) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test_gates0.v");

  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test_gates0.v");
  auto top = SNLUtils::findTop(library_);
  EXPECT_NE(nullptr, top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test_gates0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);
}