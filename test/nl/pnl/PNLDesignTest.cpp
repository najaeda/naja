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
}
