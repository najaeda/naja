// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLException.h"
#include "NLName.h"
#include "NLUniverse.h"

using namespace naja::NL;

class NLNameTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse::create();
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};


TEST_F(NLNameTest, testInternedIDs) {
  NLName name0("alpha");
  NLName name1("alpha");
  NLName name2("beta");
  NLName emptyName;

  EXPECT_EQ(name0.getID(), name1.getID());
  EXPECT_NE(name0.getID(), name2.getID());
  EXPECT_EQ(name0.getString(), "alpha");
  EXPECT_EQ(name1.getString(), "alpha");
  EXPECT_EQ(name2.getString(), "beta");
  EXPECT_EQ(emptyName.getString(), "");
  EXPECT_EQ(0, emptyName.getID());
  EXPECT_TRUE(emptyName.empty());
}

TEST_F(NLNameTest, testRequiresUniverse) {
  if (NLUniverse::get()) {
    NLUniverse::get()->destroy();
  }
  EXPECT_THROW(NLName("alpha"), NLException);
}
