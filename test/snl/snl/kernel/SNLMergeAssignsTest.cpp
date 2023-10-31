// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB0.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLInstTerm.h"
using namespace naja::SNL;

class SNLMergeAssignsTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      lib_ = SNLLibrary::create(db);
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
    SNLLibrary* lib_;
};

TEST_F(SNLMergeAssignsTest, test0) {
  auto universe = SNLUniverse::get();
  ASSERT_NE(nullptr, universe);
  universe->mergeAssigns();
}

TEST_F(SNLMergeAssignsTest, test1) {
  ASSERT_NE(nullptr, lib_);
  //create modules containing assigns
  auto design0 = SNLDesign::create(lib_);
  auto input = SNLScalarTerm::create(design0, SNLTerm::Direction::Input);
  auto inputNet = SNLScalarNet::create(design0);
  input->setNet(inputNet);
  auto output = SNLScalarTerm::create(design0, SNLTerm::Direction::Output);
  auto outputNet = SNLScalarNet::create(design0);
  output->setNet(outputNet);
  auto assign = SNLInstance::create(design0, SNLDB0::getAssign());
  assign->getInstTerm(SNLDB0::getAssignInput())->setNet(input->getNet());
  assign->getInstTerm(SNLDB0::getAssignOutput())->setNet(output->getNet());
  EXPECT_EQ(1, design0->getInstances().size());
  EXPECT_EQ(2, design0->getNets().size());
  SNLUniverse::get()->mergeAssigns();
  EXPECT_TRUE(design0->getInstances().empty());
  EXPECT_EQ(1, design0->getNets().size());
  EXPECT_EQ(inputNet, input->getNet());
  EXPECT_EQ(inputNet, output->getNet());
}
