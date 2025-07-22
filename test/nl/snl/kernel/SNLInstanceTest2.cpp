// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
using namespace std;

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLException.h"

#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
using namespace naja::NL;

class SNLInstanceTest2: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);

      auto library = NLLibrary::create(db, NLName("MYLIB"));
      design_ = SNLDesign::create(library, NLName("design"));
      model_ = SNLDesign::create(library, NLName("model"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    SNLDesign* design_;
    SNLDesign* model_;
};

TEST_F(SNLInstanceTest2, testRename) {
  auto instance0 = SNLInstance::create(design_, model_, NLName("instance0"));
  auto instance1 = SNLInstance::create(design_, model_, NLName("instance1"));
  auto instance2 = SNLInstance::create(design_, model_);
  
  EXPECT_EQ(instance0, design_->getInstance(NLName("instance0")));
  EXPECT_EQ(instance1, design_->getInstance(NLName("instance1")));
  EXPECT_FALSE(instance0->isUnnamed());
  instance0->setName(NLName());
  EXPECT_TRUE(instance0->isUnnamed());
  EXPECT_EQ(nullptr, design_->getInstance(NLName("instance0")));
  instance0->setName(NLName("instance0"));
  EXPECT_FALSE(instance0->isUnnamed());
  EXPECT_EQ(instance0, design_->getInstance(NLName("instance0")));
  EXPECT_FALSE(instance1->isUnnamed());
  instance1->setName(NLName("instance1")); //nothing should happen...
  EXPECT_EQ(instance1, design_->getInstance(NLName("instance1")));
  instance1->setName(NLName("t1"));
  EXPECT_FALSE(instance1->isUnnamed());
  EXPECT_EQ(nullptr, design_->getInstance(NLName("instance1")));
  EXPECT_EQ(instance1, design_->getInstance(NLName("t1")));
  EXPECT_TRUE(instance2->isUnnamed());
  instance2->setName(NLName("instance2"));
  EXPECT_FALSE(instance2->isUnnamed());
  EXPECT_EQ(instance2, design_->getInstance(NLName("instance2")));
  //Collision error
  EXPECT_THROW(instance2->setName(NLName("instance0")), NLException);
}

TEST_F(SNLInstanceTest2, testInstTermRenameError) {
  auto a = SNLScalarTerm::create(model_, SNLTerm::Direction::Input, NLName("a"));
  auto ins = SNLInstance::create(design_, model_, NLName("instance"));
  auto instTerm = ins->getInstTerm(a);
  EXPECT_THROW(instTerm->setName(NLName("b")), NLException);
}

TEST_F(SNLInstanceTest2, testInstTermNullTerm) {
  auto ins = SNLInstance::create(design_, model_, NLName("instance"));
  EXPECT_THROW(ins->getInstTerm(nullptr), NLException);
}
