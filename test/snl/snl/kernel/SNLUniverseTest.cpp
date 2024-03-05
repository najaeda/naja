// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
using namespace naja::SNL;

class SNLUniverseTest: public ::testing::Test {
  protected:
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLUniverseTest, testGetSNLObjects1) {
  ASSERT_EQ(nullptr, SNLUniverse::get());
  SNLUniverse::create();
  ASSERT_NE(nullptr, SNLUniverse::get());
  auto universe = SNLUniverse::get();
  ASSERT_TRUE(universe);

  EXPECT_FALSE(universe->getDBs().empty());
  EXPECT_EQ(1, universe->getDBs().size());
  EXPECT_TRUE(universe->getUserDBs().empty());
  EXPECT_EQ(0, universe->getUserDBs().size());
  EXPECT_EQ(nullptr, universe->getDesign(SNLID::DesignReference(0, 1, 1)));
  EXPECT_EQ(nullptr, universe->getTerm(SNLID::DesignObjectReference(1, 1, 2, 3)));
  EXPECT_EQ(nullptr, universe->getBusTermBit(SNLID(SNLID::Type::TermBit, 1, 1, 1, 0, 1, 1)));
  EXPECT_EQ(nullptr, universe->getNet(SNLID::DesignObjectReference(2, 3, 1, 1)));
  EXPECT_EQ(nullptr, universe->getInstance(SNLID::DesignObjectReference(1, 1, 1, 1)));
}