// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLException.h"
#include "NLLibraryTruthTables.h"
using namespace naja::SNL;

class NLLibraryTruthTablesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      primitives_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
    NLLibrary* primitives_ {};
};

TEST_F(NLLibraryTruthTablesTest, test) {
  EXPECT_EQ(NLLibraryTruthTables::LibraryTruthTables(), NLLibraryTruthTables::construct(primitives_));
  EXPECT_EQ(NLLibraryTruthTables::LibraryTruthTables(), NLLibraryTruthTables::getTruthTables(primitives_));
}
