// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLInstanceSetModelTest: public ::testing::Test {
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
      SNLParameter::create(model_, SNLName("param0"), SNLParameter::Type::Binary, "0b1010");
      SNLParameter::create(model_, SNLName("param1"), SNLParameter::Type::Decimal, "42");
      SNLParameter::create(model_, SNLName("param2"), SNLParameter::Type::Boolean, "true");

      top_ = SNLDesign::create(library, SNLName("top"));
      //Top terms
      auto topi0 = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, SNLName("topi0"));
      auto topi1 = SNLBusTerm::create(top_, SNLTerm::Direction::Input, 4, 0, SNLName("topi1"));
      auto topo0 = SNLScalarTerm::create(top_, SNLTerm::Direction::Output, SNLName("topo0"));
      
      ins0_ = SNLInstance::create(top_, model_, SNLName("ins0"));
      //instParams
      SNLInstParameter::create(ins0_, model_->getParameter(SNLName("param0")), "0b0101");
      ins1_ = SNLInstance::create(top_, model_, SNLName("ins1"));

      //nets
      auto net0 = SNLScalarNet::create(top_, SNLName("topi0"));
      topi0->setNet(net0);
      auto net1 = SNLBusNet::create(top_, 4, 0, SNLName("topi1"));
      topi1->setNet(net1);
      ins0_->getInstTerm(model_->getScalarTerm(SNLName("term0")))->setNet(net0);
      ins1_->getInstTerm(model_->getScalarTerm(SNLName("term0")))->setNet(net0);
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDesign*    model_  {nullptr};
    SNLDesign*    top_    {nullptr};
    SNLInstance*  ins0_   {nullptr};
    SNLInstance*  ins1_   {nullptr};
};

TEST_F(SNLInstanceSetModelTest, test0) {
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

TEST_F(SNLInstanceSetModelTest, testDifferentTermSizeError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term0 = newModel->getScalarTerm(SNLName("term0"));
  ASSERT_NE(nullptr, term0);
  term0->destroy();
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}

TEST_F(SNLInstanceSetModelTest, testAnonymousContradictionError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term0 = newModel->getScalarTerm(SNLName("term0"));
  ASSERT_NE(nullptr, term0);
  term0->setName(SNLName());
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermNameError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term0 = newModel->getScalarTerm(SNLName("term0"));
  ASSERT_NE(nullptr, term0);
  term0->setName(SNLName("term00"));
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermIDError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term4 = newModel->getScalarTerm(SNLName("term4"));
  ASSERT_NE(nullptr, term4);
  term4->destroy();
  SNLScalarTerm::create(newModel, SNLID::DesignObjectID(9), SNLTerm::Direction::InOut, SNLName("term4"));
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermDirectionError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term3 = newModel->getBusTerm(SNLName("term3"));
  ASSERT_NE(nullptr, term3);
  term3->setDirection(SNLTerm::Direction::Input);
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentSizeBusError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term1 = newModel->getBusTerm(SNLName("term1"));
  ASSERT_NE(nullptr, term1);
  ASSERT_EQ(1, term1->getID());
  term1->destroy();
  SNLBusTerm::create(newModel, SNLID::DesignObjectID(1), SNLTerm::Direction::Input, 5, 0, SNLName("term1"));
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermTypesError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term1 = newModel->getBusTerm(SNLName("term1"));
  ASSERT_NE(nullptr, term1);
  ASSERT_EQ(1, term1->getID());
  term1->destroy();
  SNLScalarTerm::create(newModel, SNLID::DesignObjectID(1), SNLTerm::Direction::Input, SNLName("term1"));
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentParametersSizeError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  std::string reason;
  EXPECT_TRUE(model_->deepCompare(newModel, reason, SNLDesign::CompareType::IgnoreIDAndName));
  EXPECT_TRUE(reason.empty());
  EXPECT_TRUE(newModel->isAnonymous());
  ASSERT_EQ(3, newModel->getParameters().size());
  auto param0 = newModel->getParameter(SNLName("param0"));
  ASSERT_NE(nullptr, param0);
  param0->destroy();
  ASSERT_EQ(2, newModel->getParameters().size());
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentParameterNameError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  auto param0 = newModel->getParameter(SNLName("param0"));
  ASSERT_NE(nullptr, param0);
  param0->destroy();
  SNLParameter::create(newModel, SNLName("param00"), SNLParameter::Type::Binary, "0b1010");
  EXPECT_THROW(ins0_->setModel(newModel), SNLException);  
}