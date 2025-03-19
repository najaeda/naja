// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLNetlist0.h"

#include "NLUniverse.h"

#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNetBit.h"

using namespace naja::NL;

class SNLNetlistTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      SNLNetlist0::create(db);
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
};

TEST_F(SNLNetlistTest0, test) {
  auto universe = NLUniverse::get();
  ASSERT_NE(nullptr, universe);
  ASSERT_NE(nullptr, SNLNetlist0::getDB());
  ASSERT_NE(nullptr, SNLNetlist0::getDesignsLib());
  ASSERT_NE(nullptr, SNLNetlist0::getTop());
  ASSERT_NE(nullptr, SNLNetlist0::getTopIns0());
  ASSERT_NE(nullptr, SNLNetlist0::getTopIns1());

  auto iTerm = SNLNetlist0::getTopITerm();
  ASSERT_NE(nullptr, iTerm);
  auto iNet = SNLNetlist0::getTopINet();
  ASSERT_NE(nullptr, iNet);
  for (auto termBit: iTerm->getBits()) {
    auto netBit = termBit->getNet();
    ASSERT_NE(nullptr, netBit);
    auto busNetBit = dynamic_cast<SNLBusNetBit*>(netBit);
    ASSERT_NE(nullptr, busNetBit);
    EXPECT_EQ(termBit->getBit(), busNetBit->getBit());
  }
  auto oTerm = SNLNetlist0::getTopITerm();
  ASSERT_NE(nullptr, oTerm);
  auto oNet = SNLNetlist0::getTopONet();
  ASSERT_NE(nullptr, oNet);
  for (auto termBit: oTerm->getBits()) {
    auto netBit = termBit->getNet();
    ASSERT_NE(nullptr, netBit);
    auto busNetBit = dynamic_cast<SNLBusNetBit*>(netBit);
    ASSERT_NE(nullptr, busNetBit);
    EXPECT_EQ(termBit->getBit(), busNetBit->getBit());
  }
}
