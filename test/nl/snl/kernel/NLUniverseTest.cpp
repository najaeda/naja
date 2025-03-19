// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLException.h"
using namespace naja::SNL;

class NLUniverseTest: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(NLUniverseTest, testGetSNLObjects1) {
  ASSERT_EQ(nullptr, NLUniverse::get());
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());
  auto universe = NLUniverse::get();
  ASSERT_TRUE(universe);

  EXPECT_FALSE(universe->getDBs().empty());
  EXPECT_EQ(1, universe->getDBs().size());
  EXPECT_TRUE(universe->getUserDBs().empty());
  EXPECT_EQ(0, universe->getUserDBs().size());
  EXPECT_EQ(nullptr, universe->getDesign(NLID::DesignReference(0, 1, 1)));
  EXPECT_EQ(nullptr, universe->getTerm(NLID::DesignObjectReference(1, 1, 2, 3)));
  EXPECT_EQ(nullptr, universe->getBusTermBit(NLID(NLID::Type::TermBit, 1, 1, 1, 0, 1, 1)));
  EXPECT_EQ(nullptr, universe->getNet(NLID::DesignObjectReference(2, 3, 1, 1)));
  EXPECT_EQ(nullptr, universe->getBitNet(NLID::BitNetReference(1, 1, 1, 1, 1)));
  EXPECT_EQ(nullptr, universe->getInstance(NLID::DesignObjectReference(1, 1, 1, 1)));
}

TEST_F(NLUniverseTest, testUniverseClashError) {
  ASSERT_EQ(nullptr, NLUniverse::get());
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());
  EXPECT_THROW(NLUniverse::create(), NLException);
}

TEST_F(NLUniverseTest, testEmptyUniverse) {
  ASSERT_EQ(nullptr, NLUniverse::get());
  EXPECT_EQ(nullptr, NLUniverse::getTopDB());
  EXPECT_EQ(nullptr, NLUniverse::getTopDesign());
}
