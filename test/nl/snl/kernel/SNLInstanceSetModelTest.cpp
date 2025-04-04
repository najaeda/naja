// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLException.h"

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
using namespace naja::NL;

class SNLInstanceSetModelTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      auto library = NLLibrary::create(db, NLName("MYLIB"));
      model_ = SNLDesign::create(library, NLName("model"));
      SNLScalarTerm::create(model_, SNLTerm::Direction::Input, NLName("term0"));
      SNLBusTerm::create(model_, SNLTerm::Direction::Input, 4, 0, NLName("term1"));
      SNLScalarTerm::create(model_, SNLTerm::Direction::Output, NLName("term2"));
      SNLBusTerm::create(model_, SNLTerm::Direction::Output, -2, 5, NLName("term3"));
      SNLScalarTerm::create(model_, SNLTerm::Direction::InOut, NLName("term4"));
      SNLParameter::create(model_, NLName("param0"), SNLParameter::Type::Binary, "0b1010");
      SNLParameter::create(model_, NLName("param1"), SNLParameter::Type::Decimal, "42");
      SNLParameter::create(model_, NLName("param2"), SNLParameter::Type::Boolean, "true");

      top_ = SNLDesign::create(library, NLName("top"));
      //Top terms
      auto topi0 = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("topi0"));
      auto topi1 = SNLBusTerm::create(top_, SNLTerm::Direction::Input, 4, 0, NLName("topi1"));
      auto topo0 = SNLScalarTerm::create(top_, SNLTerm::Direction::Output, NLName("topo0"));
      
      ins0_ = SNLInstance::create(top_, model_, NLName("ins0"));
      //instParams
      SNLInstParameter::create(ins0_, model_->getParameter(NLName("param0")), "0b0101");
      ins1_ = SNLInstance::create(top_, model_, NLName("ins1"));

      //nets
      auto net0 = SNLScalarNet::create(top_, NLName("topi0"));
      topi0->setNet(net0);
      auto net1 = SNLBusNet::create(top_, 4, 0, NLName("topi1"));
      topi1->setNet(net1);
      ins0_->getInstTerm(model_->getScalarTerm(NLName("term0")))->setNet(net0);
      ins1_->getInstTerm(model_->getScalarTerm(NLName("term0")))->setNet(net0);
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    SNLDesign*    model_  {nullptr};
    SNLDesign*    top_    {nullptr};
    SNLInstance*  ins0_   {nullptr};
    SNLInstance*  ins1_   {nullptr};
};

TEST_F(SNLInstanceSetModelTest, test0) {
  EXPECT_EQ(2, model_->getSlaveInstances().size());
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  EXPECT_TRUE(newModel->getSlaveInstances().empty());
  //set model
  ins0_->setModel(newModel);
  EXPECT_EQ(ins0_->getModel(), newModel);
  EXPECT_EQ(ins1_->getModel(), model_);
  EXPECT_EQ(ins0_->getInstTerms().size(), 16);
  for (auto iterm: ins0_->getInstTerms()) {
    EXPECT_EQ(iterm->getBitTerm()->getDesign(), newModel);
  }
  EXPECT_EQ(model_->getSlaveInstances().size(), 1);
  EXPECT_EQ(ins1_, *(model_->getSlaveInstances().begin()));
  EXPECT_EQ(newModel->getSlaveInstances().size(), 1);
  EXPECT_EQ(ins0_, *(newModel->getSlaveInstances().begin()));
}

TEST_F(SNLInstanceSetModelTest, testSameModel) {
  ins0_->setModel(model_);
  EXPECT_EQ(ins0_->getModel(), model_);
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermSizeError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term0 = newModel->getScalarTerm(NLName("term0"));
  ASSERT_NE(nullptr, term0);
  term0->destroy();
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}

TEST_F(SNLInstanceSetModelTest, testAnonymousContradictionError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term0 = newModel->getScalarTerm(NLName("term0"));
  ASSERT_NE(nullptr, term0);
  term0->setName(NLName());
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermNameError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term0 = newModel->getScalarTerm(NLName("term0"));
  ASSERT_NE(nullptr, term0);
  term0->setName(NLName("term00"));
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermIDError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term4 = newModel->getScalarTerm(NLName("term4"));
  ASSERT_NE(nullptr, term4);
  term4->destroy();
  SNLScalarTerm::create(newModel, NLID::DesignObjectID(9), SNLTerm::Direction::InOut, NLName("term4"));
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermDirectionError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term3 = newModel->getBusTerm(NLName("term3"));
  ASSERT_NE(nullptr, term3);
  term3->setDirection(SNLTerm::Direction::Input);
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentSizeBusError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term1 = newModel->getBusTerm(NLName("term1"));
  ASSERT_NE(nullptr, term1);
  ASSERT_EQ(1, term1->getID());
  term1->destroy();
  SNLBusTerm::create(newModel, NLID::DesignObjectID(1), SNLTerm::Direction::Input, 5, 0, NLName("term1"));
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentTermTypesError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  ASSERT_EQ(5, newModel->getTerms().size());
  auto term1 = newModel->getBusTerm(NLName("term1"));
  ASSERT_NE(nullptr, term1);
  ASSERT_EQ(1, term1->getID());
  term1->destroy();
  SNLScalarTerm::create(newModel, NLID::DesignObjectID(1), SNLTerm::Direction::Input, NLName("term1"));
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentParametersSizeError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  std::string reason;
  EXPECT_TRUE(model_->deepCompare(newModel, reason, NLDesign::CompareType::IgnoreIDAndName));
  EXPECT_TRUE(reason.empty());
  EXPECT_EQ(std::string(), reason);
  EXPECT_TRUE(newModel->isAnonymous());
  ASSERT_EQ(3, newModel->getParameters().size());
  auto param0 = newModel->getParameter(NLName("param0"));
  ASSERT_NE(nullptr, param0);
  param0->destroy();
  ASSERT_EQ(2, newModel->getParameters().size());
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}

TEST_F(SNLInstanceSetModelTest, testDifferentParameterNameError) {
  //clone model
  auto newModel = model_->clone();
  ASSERT_NE(newModel, nullptr);
  auto param0 = newModel->getParameter(NLName("param0"));
  ASSERT_NE(nullptr, param0);
  param0->destroy();
  SNLParameter::create(newModel, NLName("param00"), SNLParameter::Type::Binary, "0b1010");
  EXPECT_THROW(ins0_->setModel(newModel), NLException);  
}
