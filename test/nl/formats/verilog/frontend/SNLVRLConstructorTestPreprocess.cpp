// SPDX-FileCopyrightText: 2026 The Naja authors
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>

#include "NLUniverse.h"
#include "SNLUtils.h"
#include "SNLBusTerm.h"

#include "SNLVRLConstructor.h"
#include "SNLVRLConstructorException.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestPreprocess: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      library_ = NLLibrary::create(db_, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
      db_ = nullptr;
    }
  protected:
    NLDB*       db_ {nullptr};
    NLLibrary*  library_ {nullptr};
};

TEST_F(SNLVRLConstructorTestPreprocess, testPreprocessEnabled) {
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  auto path = benchmarksPath/"preprocess_top.v";
  {
    SNLVRLConstructor constructor(library_);
    EXPECT_THROW(constructor.construct(naja::verilog::VerilogConstructor::Paths{path}), SNLVRLConstructorException);
  }

  auto lib2 = NLLibrary::create(db_, NLName("MYLIB2"));
  SNLVRLConstructor constructor(lib2);
  constructor.config_.preprocessEnabled_ = true;
  constructor.construct(naja::verilog::VerilogConstructor::Paths{path});

  auto top = SNLUtils::findTop(lib2);
  ASSERT_NE(top, nullptr);
  EXPECT_EQ("top", top->getName().getString());
  EXPECT_EQ(1, top->getInstances().size());

  auto child = lib2->getSNLDesign(NLName("child"));
  ASSERT_NE(child, nullptr);

  auto topA = top->getBusTerm(NLName("a"));
  ASSERT_NE(topA, nullptr);
  EXPECT_EQ(2, topA->getWidth());
  EXPECT_EQ(1, topA->getMSB());
  EXPECT_EQ(0, topA->getLSB());

  auto childA = child->getBusTerm(NLName("a"));
  ASSERT_NE(childA, nullptr);
  EXPECT_EQ(2, childA->getWidth());
  EXPECT_EQ(1, childA->getMSB());
  EXPECT_EQ(0, childA->getLSB());
}
