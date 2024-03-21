// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
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

class SNLNullModelInstanceTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);

      auto library = SNLLibrary::create(db, SNLName("MYLIB"));
      design_ = SNLDesign::create(library, SNLName("design"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDesign* design_;
};

TEST_F(SNLNullModelInstanceTest0, test0) {
  auto instance = SNLInstance::create(design_, SNLName("instance"));
  ASSERT_NE(nullptr, instance);
  EXPECT_FALSE(instance->isBound());
  EXPECT_TRUE(instance->getInstTerms().empty());
  EXPECT_EQ(design_->getInstances().size(), 1);
  //auto instTerm = SNLInstTerm::create(instance, SNLName("instTerm"));
}