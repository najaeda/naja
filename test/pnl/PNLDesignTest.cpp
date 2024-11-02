// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLLibrary.h"
#include "PNLDesign.h"
using namespace naja::SNL;

class PNLDesignTest: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDB*  db_;
};

TEST_F(PNLDesignTest, testCreation0) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getSNLDesigns().size());
  EXPECT_TRUE(library->getSNLDesigns().empty());
  SNLDesign* snlDesign = SNLDesign::create(library, SNLName("design"));
  ASSERT_NE(snlDesign, nullptr);
  PNLDesign* pnlDesign = PNLDesign::create(library);
  ASSERT_NE(pnlDesign, nullptr);
}  