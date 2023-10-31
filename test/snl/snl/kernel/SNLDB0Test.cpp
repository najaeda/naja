// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB0.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLDB0Test: public ::testing::Test {
  protected:
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDB0Test, testAssign) {
  SNLUniverse::create();
  ASSERT_NE(nullptr, SNLUniverse::get());

  auto db0 = SNLUniverse::get()->getDB(0);
  ASSERT_NE(nullptr, db0);
  EXPECT_TRUE(SNLUniverse::isDB0(db0));
  EXPECT_EQ(SNLDB0::getDB0(), db0);
  EXPECT_EQ(SNLUniverse::getDB0(), db0);

  auto assign = SNLDB0::getAssign();
  ASSERT_NE(nullptr, assign);
  EXPECT_TRUE(SNLDB0::isDB0Primitive(assign));
  EXPECT_EQ(0, assign->getID());
  auto assignInput = SNLDB0::getAssignInput();
  ASSERT_NE(nullptr, assignInput);
  EXPECT_EQ(0, assignInput->getID());
  auto assignOutput = SNLDB0::getAssignOutput();
  ASSERT_NE(nullptr, assignOutput);
  EXPECT_EQ(1, assignOutput->getID());
  SNLBitNet* assignFT = assignInput->getNet();
  ASSERT_NE(nullptr, assignFT);
  EXPECT_EQ(assignOutput->getNet(), assignFT);

  auto db1 = SNLDB::create(SNLUniverse::get());
  EXPECT_FALSE(SNLUniverse::isDB0(db1));

  EXPECT_EQ(nullptr, SNLDB0::getANDOutput(assign));
  EXPECT_EQ(nullptr, SNLDB0::getANDInputs(assign));

  EXPECT_FALSE(SNLDB0::getDB0()->isTopDB());
  EXPECT_THROW(SNLUniverse::get()->setTopDB(db0), SNLException);
}

TEST_F(SNLDB0Test, testAND) {
  SNLUniverse::create();
  ASSERT_NE(nullptr, SNLUniverse::get());

  SNLDesign* and2 = SNLDB0::getAND(2);
  ASSERT_NE(nullptr, and2);
  SNLScalarTerm* and2Out = SNLDB0::getANDOutput(and2);
  ASSERT_NE(nullptr, and2Out);
  EXPECT_EQ(SNLID::DesignObjectID(0), and2Out->getID());
  EXPECT_EQ(SNLTerm::Direction::Output, and2Out->getDirection());
  SNLBusTerm* and2Inputs = SNLDB0::getANDInputs(and2);
  ASSERT_NE(nullptr, and2Inputs);
  EXPECT_EQ(SNLID::DesignObjectID(1), and2Inputs->getID());
  EXPECT_EQ(SNLTerm::Direction::Input, and2Inputs->getDirection());
  EXPECT_EQ(2, and2Inputs->getSize());

  SNLDesign* and2Test = SNLDB0::getAND(2);
  EXPECT_EQ(and2, and2Test);

  SNLDesign* and48 = SNLDB0::getAND(48);
  ASSERT_NE(nullptr, and48);
  SNLScalarTerm* and48Out = SNLDB0::getANDOutput(and48);
  ASSERT_NE(nullptr, and48Out);
  EXPECT_EQ(SNLID::DesignObjectID(0), and48Out->getID());
  EXPECT_EQ(SNLTerm::Direction::Output, and48Out->getDirection());
  SNLBusTerm* and48Inputs = SNLDB0::getANDInputs(and48);
  ASSERT_NE(nullptr, and48Inputs);
  EXPECT_EQ(SNLID::DesignObjectID(1), and48Inputs->getID());
  EXPECT_EQ(SNLTerm::Direction::Input, and48Inputs->getDirection());
  EXPECT_EQ(48, and48Inputs->getSize());
}

TEST_F(SNLDB0Test, testNULLUniverse) {
  EXPECT_EQ(nullptr, SNLUniverse::get());
  EXPECT_FALSE(SNLUniverse::isDB0(nullptr));
  EXPECT_EQ(nullptr, SNLDB0::getAssign());
  EXPECT_EQ(nullptr, SNLDB0::getAssignInput());
  EXPECT_EQ(nullptr, SNLDB0::getAssignOutput());
  EXPECT_FALSE(SNLDB0::getANDLibrary());
  EXPECT_EQ(nullptr, SNLDB0::getAND(2));
}