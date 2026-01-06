// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include <filesystem>
#include <fstream>

#include "NLUniverse.h"

#include "SNLVRLConstructor.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestImplicitNets: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTestImplicitNets, test0) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"implicit_nets.v");
  auto test0 = library_->getSNLDesign(NLName("implicit_net0"));

  ASSERT_NE(test0, nullptr);
  EXPECT_TRUE(test0->getBusNets().empty());
  EXPECT_EQ(6, test0->getNets().size()); // 1'b0, 1'b1, a, b, c, y
  EXPECT_EQ(6, test0->getScalarNets().size()); // 1'b0, 1'b1, a, b, c, y
  using ScalarNets = std::vector<SNLScalarNet*>;
  ScalarNets scalarNets(test0->getScalarNets().begin(), test0->getScalarNets().end());
  EXPECT_THAT(scalarNets,
    ElementsAre(
      test0->getScalarNet(0),
      test0->getScalarNet(1),
      test0->getScalarNet(NLName("a")),
      test0->getScalarNet(NLName("b")),
      test0->getScalarNet(NLName("y")),
      test0->getScalarNet(NLName("c")) // c is implicit net and last created
    )
  );
}