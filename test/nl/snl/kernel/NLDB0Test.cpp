// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "NLException.h"
using namespace naja::NL;

class NLDB0Test: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(NLDB0Test, testAssign) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  auto db0 = NLUniverse::get()->getDB(0);
  ASSERT_NE(nullptr, db0);
  EXPECT_TRUE(NLUniverse::isDB0(db0));
  EXPECT_EQ(NLDB0::getDB0(), db0);
  EXPECT_EQ(NLUniverse::getDB0(), db0);

  auto assign = NLDB0::getAssign();
  ASSERT_NE(nullptr, assign);
  EXPECT_TRUE(NLDB0::isDB0Primitive(assign));
  EXPECT_EQ(0, assign->getID());
  auto assignInput = NLDB0::getAssignInput();
  ASSERT_NE(nullptr, assignInput);
  EXPECT_EQ(0, assignInput->getID());
  auto assignOutput = NLDB0::getAssignOutput();
  ASSERT_NE(nullptr, assignOutput);
  EXPECT_EQ(1, assignOutput->getID());
  SNLBitNet* assignFT = assignInput->getNet();
  ASSERT_NE(nullptr, assignFT);
  EXPECT_EQ(assignOutput->getNet(), assignFT);

  auto db1 = NLDB::create(NLUniverse::get());
  EXPECT_FALSE(NLUniverse::isDB0(db1));

  //EXPECT_EQ(nullptr, NLDB0::getANDOutput(assign));
  //EXPECT_EQ(nullptr, NLDB0::getANDInputs(assign));

  EXPECT_FALSE(NLDB0::getDB0()->isTopDB());
  EXPECT_THROW(NLUniverse::get()->setTopDB(db0), NLException);
}

TEST_F(NLDB0Test, testAND) {
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());

  SNLDesign* and2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
  ASSERT_NE(nullptr, and2);
  SNLScalarTerm* and2Out = NLDB0::getGateSingleTerm(and2);
  ASSERT_NE(nullptr, and2Out);
  EXPECT_EQ(NLID::DesignObjectID(0), and2Out->getID());
  EXPECT_EQ(SNLTerm::Direction::Output, and2Out->getDirection());
  SNLBusTerm* and2Inputs = NLDB0::getGateNTerms(and2);
  ASSERT_NE(nullptr, and2Inputs);
  EXPECT_EQ(NLID::DesignObjectID(1), and2Inputs->getID());
  EXPECT_EQ(SNLTerm::Direction::Input, and2Inputs->getDirection());
  EXPECT_EQ(2, and2Inputs->getWidth());

  SNLDesign* and2Test = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
  EXPECT_EQ(and2, and2Test);

  SNLDesign* and48 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 48);
  ASSERT_NE(nullptr, and48);
  SNLScalarTerm* and48Out = NLDB0::getGateSingleTerm(and48);
  ASSERT_NE(nullptr, and48Out);
  EXPECT_EQ(NLID::DesignObjectID(0), and48Out->getID());
  EXPECT_EQ(SNLTerm::Direction::Output, and48Out->getDirection());
  SNLBusTerm* and48Inputs = NLDB0::getGateNTerms(and48);
  ASSERT_NE(nullptr, and48Inputs);
  EXPECT_EQ(NLID::DesignObjectID(1), and48Inputs->getID());
  EXPECT_EQ(SNLTerm::Direction::Input, and48Inputs->getDirection());
  EXPECT_EQ(48, and48Inputs->getWidth());
}

TEST_F(NLDB0Test, testNULLUniverse) {
  EXPECT_EQ(nullptr, NLUniverse::get());
  EXPECT_FALSE(NLUniverse::isDB0(nullptr));
  EXPECT_EQ(nullptr, NLDB0::getAssign());
  EXPECT_EQ(nullptr, NLDB0::getAssignInput());
  EXPECT_EQ(nullptr, NLDB0::getAssignOutput());
  EXPECT_EQ(nullptr, NLDB0::getGateLibrary(NLDB0::GateType::And));
  EXPECT_EQ(nullptr, NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2));
}

TEST_F(NLDB0Test, testErrors) {
  EXPECT_FALSE(NLDB0::isNInputGate(nullptr));
  EXPECT_FALSE(NLDB0::isNOutputGate(nullptr));
  EXPECT_FALSE(NLDB0::isGate(nullptr));

  auto universe = NLUniverse::create();
  ASSERT_NE(nullptr, universe);
  auto db = NLDB::create(universe);
  ASSERT_NE(nullptr, db);
  auto library = NLLibrary::create(db, NLName("testLibrary"));

  auto model = SNLDesign::create(library, NLName("testGate"));
  EXPECT_FALSE(NLDB0::isNInputGate(model));
  EXPECT_FALSE(NLDB0::isNOutputGate(model));
  EXPECT_FALSE(NLDB0::isGate(model));
  EXPECT_EQ(std::string(), NLDB0::getGateName(model));
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::And, 0), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::Buf, 0), NLException);
  NLUniverse::get()->destroy();
  EXPECT_THROW(NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Unknown, 2), NLException);
  EXPECT_THROW(NLDB0::getOrCreateNInputGate(NLDB0::GateType::Unknown, 2), NLException);
}