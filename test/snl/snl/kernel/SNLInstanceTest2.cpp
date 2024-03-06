// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
using namespace std;

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLInstanceTest2: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);

      auto library = SNLLibrary::create(db, SNLName("MYLIB"));
      design_ = SNLDesign::create(library, SNLName("design"));
      model_ = SNLDesign::create(library, SNLName("model"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDesign* design_;
    SNLDesign* model_;
};

TEST_F(SNLInstanceTest2, testRename) {
  auto instance0 = SNLInstance::create(design_, model_, SNLName("instance0"));
  auto instance1 = SNLInstance::create(design_, model_, SNLName("instance1"));
  auto instance2 = SNLInstance::create(design_, model_);
  
  EXPECT_EQ(instance0, design_->getInstance(SNLName("instance0")));
  EXPECT_EQ(instance1, design_->getInstance(SNLName("instance1")));
  EXPECT_FALSE(instance0->isAnonymous());
  instance0->setName(SNLName());
  EXPECT_TRUE(instance0->isAnonymous());
  EXPECT_EQ(nullptr, design_->getInstance(SNLName("instance0")));
  instance0->setName(SNLName("instance0"));
  EXPECT_FALSE(instance0->isAnonymous());
  EXPECT_EQ(instance0, design_->getInstance(SNLName("instance0")));
  EXPECT_FALSE(instance1->isAnonymous());
  instance1->setName(SNLName("instance1")); //nothing should happen...
  EXPECT_EQ(instance1, design_->getInstance(SNLName("instance1")));
  instance1->setName(SNLName("t1"));
  EXPECT_FALSE(instance1->isAnonymous());
  EXPECT_EQ(nullptr, design_->getInstance(SNLName("instance1")));
  EXPECT_EQ(instance1, design_->getInstance(SNLName("t1")));
  EXPECT_TRUE(instance2->isAnonymous());
  instance2->setName(SNLName("instance2"));
  EXPECT_FALSE(instance2->isAnonymous());
  EXPECT_EQ(instance2, design_->getInstance(SNLName("instance2")));
  //Collision error
  EXPECT_THROW(instance2->setName(SNLName("instance0")), SNLException);
}


TEST_F(SNLInstanceTest2, testInstTermRenameError) {
  auto a = SNLScalarTerm::create(model_, SNLTerm::Direction::Input, SNLName("a"));
  auto ins = SNLInstance::create(design_, model_, SNLName("instance"));
  auto instTerm = ins->getInstTerm(a);
  EXPECT_THROW(instTerm->setName(SNLName("b")), SNLException);
}