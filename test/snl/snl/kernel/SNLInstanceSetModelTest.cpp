// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
using namespace naja::SNL;

class SNLDInstanceSetModelTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto library = SNLLibrary::create(db, SNLName("MYLIB"));
      model_ = SNLDesign::create(library, SNLName("model"));
      SNLScalarTerm::create(model_, SNLTerm::Direction::Input, SNLName("term0"));
      SNLBusTerm::create(model_, SNLTerm::Direction::Input, 4, 0, SNLName("term1"));
      SNLScalarTerm::create(model_, SNLTerm::Direction::Output, SNLName("term2"));
      SNLBusTerm::create(model_, SNLTerm::Direction::Output, -2, 5, SNLName("term3"));
      SNLScalarTerm::create(model_, SNLTerm::Direction::InOut, SNLName("term4"));
      //parameters_.push_back(SNLParameter::create(design_, SNLName("param0"), SNLParameter::Type::Binary, "0b1010"));
      //parameters_.push_back(SNLParameter::create(design_, SNLName("param1"), SNLParameter::Type::Decimal, "42"));
      //parameters_.push_back(SNLParameter::create(design_, SNLName("param2"), SNLParameter::Type::Boolean, "true"));

      top_ = SNLDesign::create(library, SNLName("top"));
      ins0_ = SNLInstance::create(top_, model_, SNLName("ins0"));
      ins1_ = SNLInstance::create(top_, model_, SNLName("ins1"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDesign*    model_  {nullptr};
    SNLDesign*    top_    {nullptr};
    SNLInstance*  ins0_   {nullptr};
    SNLInstance*  ins1_   {nullptr};
};

TEST_F(SNLDInstanceSetModelTest, test0) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  //set model
  ins0_->setModel(newModel);
  EXPECT_EQ(ins0_->getModel(), newModel);
  EXPECT_EQ(ins1_->getModel(), model_);
  EXPECT_EQ(ins0_->getInstTerms().size(), 16);
  for (auto iterm: ins0_->getInstTerms()) {
    EXPECT_EQ(iterm->getBitTerm()->getDesign(), newModel);
  }
}