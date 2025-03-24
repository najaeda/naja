// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "PNLDesign.h"
using namespace naja::NL;

class PNLDesignTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLDB*  db_;
};

TEST_F(PNLDesignTest, testCreation0) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getPNLDesigns().size());
  EXPECT_TRUE(library->getPNLDesigns().empty());
  EXPECT_EQ(nullptr, library->getPNLDesign(0));
  EXPECT_EQ(nullptr, library->getPNLDesign(NLName("design")));

  auto design0 = PNLDesign::create(library, NLName("design0"));
  ASSERT_NE(design0, nullptr);
  EXPECT_EQ(NLName("design0"), design0->getName());
  EXPECT_EQ(0, design0->getID());
  EXPECT_FALSE(design0->isAnonymous());
  EXPECT_EQ(design0, library->getPNLDesign(0));
  EXPECT_EQ(design0, library->getPNLDesign(NLName("design0")));

  auto design1 = PNLDesign::create(library, NLName("design1"));
  ASSERT_NE(design1, nullptr);
  EXPECT_EQ(NLName("design1"), design1->getName());
  EXPECT_EQ(1, design1->getID());
  EXPECT_FALSE(design1->isAnonymous());
  EXPECT_EQ(design1, library->getPNLDesign(1));
  EXPECT_EQ(design1, library->getPNLDesign(NLName("design1"))); 
}
