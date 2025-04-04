// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;
using ::testing::TypedEq;

#include <filesystem>
#include <fstream>

#include "NLUniverse.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLUtils.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"

#include "SNLPyLoader.h"
#include "SNLVRLConstructor.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTest2: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTest2, test) {
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto primitivesPath = std::filesystem::path(SNL_VRL_BENCHMARKS_PATH);
  primitivesPath /= "primitives0.py";
  SNLPyLoader::loadPrimitives(prims, primitivesPath);
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test1.v");

  ASSERT_EQ(3, library_->getSNLDesigns().size());
  auto model0 = library_->getSNLDesign(NLName("model0"));
  ASSERT_NE(model0, nullptr);
  ASSERT_EQ(3, model0->getTerms().size());
  using Terms = std::vector<SNLTerm*>;
  Terms terms(model0->getTerms().begin(), model0->getTerms().end());
  EXPECT_THAT(terms,
    ElementsAre(
      model0->getTerm(NLName("io")),
      model0->getTerm(NLName("o")),
      model0->getTerm(NLName("i"))
    )
  );

  auto model1 = library_->getSNLDesign(NLName("model1"));
  ASSERT_NE(model1, nullptr);
  ASSERT_EQ(3, model1->getTerms().size());
  terms = Terms(model1->getTerms().begin(), model1->getTerms().end());
  EXPECT_THAT(terms,
    ElementsAre(
      model1->getTerm(NLName("i")),
      model1->getTerm(NLName("o")),
      model1->getTerm(NLName("io"))
    )
  );

  auto test = library_->getSNLDesign(NLName("test"));
  ASSERT_NE(test, nullptr);

  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test1.v");
  auto top = SNLUtils::findTop(library_);
  EXPECT_EQ(top, test);
  using Instances = std::vector<SNLInstance*>;
  Instances instances(top->getInstances().begin(), top->getInstances().end());
  //6 standard instances, 3 assigns
  ASSERT_EQ(9, instances.size());
  ASSERT_EQ(1, model0->getInstances().size());
  auto lut = model0->getInstance(NLName("lut"));
  ASSERT_NE(lut, nullptr);
  EXPECT_EQ("lut", lut->getName().getString());
  auto lutModel = lut->getModel();
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
}
