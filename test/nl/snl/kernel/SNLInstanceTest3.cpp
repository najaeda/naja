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

class SNLInstanceTest3: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);

      library_ = NLLibrary::create(db, NLName("MYLIB"));
      design_ = SNLDesign::create(library_, NLName("design"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLLibrary*  library_;
    SNLDesign*  design_;
};

TEST_F(SNLInstanceTest3, testBlackBoxes) {
  auto userBlackBox = SNLDesign::create(library_, SNLDesign::Type::UserBlackBox, NLName("user_blackbox"));
  auto autoBlackBox = SNLDesign::create(library_, SNLDesign::Type::AutoBlackBox, NLName("auto_blackbox"));
  auto instance0 = SNLInstance::create(design_, userBlackBox, NLName("instance0"));
  auto instance1 = SNLInstance::create(design_, autoBlackBox, NLName("instance1"));

  EXPECT_TRUE(userBlackBox->isUserBlackBox());
  EXPECT_TRUE(autoBlackBox->isAutoBlackBox());
  EXPECT_TRUE(userBlackBox->isBlackBox());
  EXPECT_TRUE(autoBlackBox->isBlackBox());
  EXPECT_TRUE(userBlackBox->isLeaf());
  EXPECT_TRUE(autoBlackBox->isLeaf());
  EXPECT_TRUE(instance0->isUserBlackBox());
  EXPECT_TRUE(instance0->isBlackBox());
  EXPECT_TRUE(instance1->isAutoBlackBox());
  EXPECT_TRUE(instance1->isBlackBox());
}