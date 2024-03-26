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
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLUtils.h"
#include "SNLException.h"
using namespace naja::SNL;

namespace {

void compareTerms(const SNLDesign* design, const SNLDesign* newDesign) {
  ASSERT_EQ(design->getTerms().size(), newDesign->getTerms().size());
  for (auto term: design->getTerms()) {
    auto found = newDesign->getTerm(term->getID());
    ASSERT_NE(nullptr, found);
    EXPECT_EQ(term->getID(), found->getID());
    EXPECT_EQ(term->getDirection(), found->getDirection());
    EXPECT_EQ(term->getName(), found->getName());
    EXPECT_EQ(term->getSize(), found->getSize());
    if (not found->isAnonymous()) {
      EXPECT_EQ(found, newDesign->getTerm(found->getName()));
    }
  }
}

void compareParameters(const SNLDesign* design, const SNLDesign* newDesign) {
  ASSERT_EQ(design->getParameters().size(), newDesign->getParameters().size());
  for (auto parameter: design->getParameters()) {
    auto found = newDesign->getParameter(parameter->getName());
    ASSERT_NE(nullptr, found);
    EXPECT_EQ(parameter->getType(), found->getType());
    EXPECT_EQ(parameter->getName(), found->getName());
    EXPECT_EQ(parameter->getValue(), found->getValue());
  }
}

void compareInstParameters(const SNLInstance* instance, const SNLInstance* newInstance) {
  ASSERT_EQ(instance->getInstParameters().size(), newInstance->getInstParameters().size());
  for (auto instParameter: instance->getInstParameters()) {
    auto found = newInstance->getInstParameter(instParameter->getName());
    ASSERT_NE(nullptr, found);
    EXPECT_EQ(instParameter->getParameter(), found->getParameter());
    EXPECT_EQ(instParameter->getValue(), found->getValue());
  }
}

void compareInstances(const SNLDesign* design, const SNLDesign* newDesign) {
  ASSERT_EQ(design->getInstances().size(), newDesign->getInstances().size());
  for (auto instance: design->getInstances()) {
    auto found = newDesign->getInstance(instance->getID());
    ASSERT_NE(nullptr, found);
    EXPECT_EQ(instance->getID(), found->getID());
    EXPECT_EQ(instance->getModel(), found->getModel());
    EXPECT_EQ(instance->getName(), found->getName());
    if (not found->isAnonymous()) {
      EXPECT_EQ(found, newDesign->getInstance(found->getName()));
    }
    compareInstParameters(instance, found);
  }
}

void compareNets(const SNLDesign* design, const SNLDesign* newDesign) {
  ASSERT_EQ(design->getNets().size(), newDesign->getNets().size());
  for (auto net: design->getNets()) {
    auto found = newDesign->getNet(net->getID());
    ASSERT_NE(nullptr, found);
    EXPECT_EQ(net->getID(), found->getID());
    EXPECT_EQ(net->getName(), found->getName());
  }
}

} // namespace

class SNLDesignCloneTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto primitives = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMITIVES"));
      auto prim0 = SNLDesign::create(primitives, SNLDesign::Type::Primitive, SNLName("prim0"));
      auto param0 = SNLParameter::create(prim0, SNLName("param0"), SNLParameter::Type::Binary, "0b1010");
      auto param1 = SNLParameter::create(prim0, SNLName("param1"), SNLParameter::Type::Decimal, "42");
      auto param2 = SNLParameter::create(prim0, SNLName("param2"), SNLParameter::Type::Boolean, "true");
      auto prim0Term0 = SNLScalarTerm::create(prim0, SNLTerm::Direction::Input, SNLName("term0"));
      auto prim0Term1 = SNLScalarTerm::create(prim0, SNLTerm::Direction::Input, SNLName("term1"));
      auto prim0Term2 = SNLBusTerm::create(prim0, SNLTerm::Direction::Output, 4, 0, SNLName("term2"));
      auto prim1 = SNLDesign::create(primitives, SNLDesign::Type::Primitive, SNLName("prim1"));
      auto prim1Term0 = SNLScalarTerm::create(prim1, SNLTerm::Direction::Input, SNLName("term0"));
      auto prim1Term1 = SNLScalarTerm::create(prim1, SNLTerm::Direction::Output, SNLName("term1"));
      auto library = SNLLibrary::create(db, SNLName("MYLIB"));
      design_ = SNLDesign::create(library, SNLName("design"));
      terms_.push_back(SNLScalarTerm::create(design_, SNLTerm::Direction::Input, SNLName("term0")));
      terms_.push_back(SNLBusTerm::create(design_, SNLTerm::Direction::Input, 4, 0, SNLName("term1")));
      terms_.push_back(SNLScalarTerm::create(design_, SNLTerm::Direction::Output, SNLName("term2")));
      terms_.push_back(SNLBusTerm::create(design_, SNLTerm::Direction::Output, -2, 5, SNLName("term3")));
      terms_.push_back(SNLScalarTerm::create(design_, SNLTerm::Direction::InOut, SNLName("term4")));
      parameters_.push_back(SNLParameter::create(design_, SNLName("param0"), SNLParameter::Type::Binary, "0b1010"));
      parameters_.push_back(SNLParameter::create(design_, SNLName("param1"), SNLParameter::Type::Decimal, "42"));
      parameters_.push_back(SNLParameter::create(design_, SNLName("param2"), SNLParameter::Type::Boolean, "true"));
      
      instances_.push_back(SNLInstance::create(design_, prim0, SNLName("inst0")));
      SNLInstParameter::create(instances_[0], param0, "0b1100");
      SNLInstParameter::create(instances_[0], param1, "43");
      SNLInstParameter::create(instances_[0], param2, "false");
      instances_.push_back(SNLInstance::create(design_, prim1, SNLName("inst1")));
      instances_.push_back(SNLInstance::create(design_, prim0, SNLName("inst2")));
      instances_.push_back(SNLInstance::create(design_, prim1, SNLName("inst3")));
      nets_.push_back(SNLScalarNet::create(design_, SNLName("net0")));
      nets_.push_back(SNLBusNet::create(design_, -2, 4, SNLName("net1")));
      nets_.push_back(SNLScalarNet::create(design_, SNLName("net2")));
      terms_[0]->setNet(nets_[0]);
      ((SNLBusTerm*)terms_[3])->getBit(0)->setNet(nets_[0]);
      instances_[0]->getInstTerm(prim0Term0)->setNet(nets_[0]);
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    using Terms = std::vector<SNLTerm*>;
    using Parameters = std::vector<SNLParameter*>;
    using Instances = std::vector<SNLInstance*>;
    using Nets = std::vector<SNLNet*>;
    SNLDesign*  design_     {nullptr};
    Terms       terms_      {};
    Parameters  parameters_ {};
    Instances   instances_  {};
    Nets        nets_       {};
};

TEST_F(SNLDesignCloneTest, testcloneInterface0) {
  auto newDesign = design_->cloneInterface();
  ASSERT_NE(nullptr, newDesign);
  EXPECT_TRUE(newDesign->isAnonymous());
  EXPECT_EQ(design_->getLibrary(), newDesign->getLibrary());
  EXPECT_EQ(newDesign, design_->getLibrary()->getDesign(newDesign->getID()));
  compareTerms(design_, newDesign);
  compareParameters(design_, newDesign);
  EXPECT_TRUE(newDesign->getInstances().empty());
  EXPECT_TRUE(newDesign->getNets().empty());
}

TEST_F(SNLDesignCloneTest, testCloneInterface1) {
  auto newDesign = design_->cloneInterface(SNLName("newDesign"));
  ASSERT_NE(nullptr, newDesign);
  EXPECT_FALSE(newDesign->isAnonymous());
  EXPECT_EQ(SNLName("newDesign"), newDesign->getName());
  EXPECT_EQ(design_->getLibrary(), newDesign->getLibrary());
  EXPECT_EQ(newDesign, design_->getLibrary()->getDesign(newDesign->getID()));
  EXPECT_EQ(newDesign, design_->getLibrary()->getDesign(newDesign->getName()));
  compareTerms(design_, newDesign);
  compareParameters(design_, newDesign);
  EXPECT_TRUE(newDesign->getInstances().empty());
  EXPECT_TRUE(newDesign->getNets().empty());
}

TEST_F(SNLDesignCloneTest, testCloneInterface2) {
  auto newLibrary = SNLLibrary::create(design_->getLibrary()->getDB(), SNLName("newLibrary"));
  auto newDesign = design_->cloneInterfaceToLibrary(newLibrary, SNLName("newDesign"));
  ASSERT_NE(nullptr, newDesign);
  EXPECT_FALSE(newDesign->isAnonymous());
  EXPECT_EQ(SNLName("newDesign"), newDesign->getName());
  EXPECT_EQ(newLibrary, newDesign->getLibrary());
  EXPECT_EQ(newDesign, newLibrary->getDesign(newDesign->getID()));
  EXPECT_EQ(newDesign, newLibrary->getDesign(newDesign->getName()));
  compareTerms(design_, newDesign);
  compareParameters(design_, newDesign);
  EXPECT_TRUE(newDesign->getInstances().empty());
  EXPECT_TRUE(newDesign->getNets().empty());
}

TEST_F(SNLDesignCloneTest, testClone0) {
  auto newDesign = design_->clone();
  ASSERT_NE(nullptr, newDesign);
  EXPECT_TRUE(newDesign->isAnonymous());
  EXPECT_EQ(design_->getLibrary(), newDesign->getLibrary());
  EXPECT_EQ(newDesign, design_->getLibrary()->getDesign(newDesign->getID()));
  compareTerms(design_, newDesign);
  compareParameters(design_, newDesign);
  compareInstances(design_, newDesign);
  compareNets(design_, newDesign);
}

TEST_F(SNLDesignCloneTest, testCloneCompare) {
  auto newDesign = design_->clone();
  std::string reason;
  EXPECT_TRUE(newDesign->deepCompare(design_, reason, SNLDesign::CompareType::IgnoreIDAndName));
  EXPECT_TRUE(reason.empty());
}

TEST_F(SNLDesignCloneTest, testErrors) {
  EXPECT_THROW(design_->clone(design_->getName()), SNLException);
}