// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLLibraryTruthTables.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLLibraryTruthTablesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      primitives_ = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
    SNLLibrary* primitives_ {};
};

TEST_F(SNLLibraryTruthTablesTest, test) {
  EXPECT_EQ(SNLLibraryTruthTables::LibraryTruthTables(), SNLLibraryTruthTables::construct(primitives_));
  EXPECT_EQ(SNLLibraryTruthTables::LibraryTruthTables(), SNLLibraryTruthTables::getTruthTables(primitives_));
}