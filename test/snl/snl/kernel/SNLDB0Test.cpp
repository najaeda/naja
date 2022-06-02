#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB0.h"
#include "SNLScalarTerm.h"
using namespace naja::SNL;

class SNLDB0Test: public ::testing::Test {
  protected:
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
};

TEST_F(SNLDB0Test, testAssign) {
  EXPECT_EQ(nullptr, SNLUniverse::getAssign());
  EXPECT_EQ(nullptr, SNLUniverse::getAssignInput());
  EXPECT_EQ(nullptr, SNLUniverse::getAssignOutput());
  EXPECT_FALSE(SNLUniverse::isDB0(nullptr));

  SNLUniverse::create();
  ASSERT_NE(nullptr, SNLUniverse::get());

  auto db0 = SNLUniverse::get()->getDB(0);
  ASSERT_NE(nullptr, db0);
  EXPECT_TRUE(SNLUniverse::isDB0(db0));

  auto assign = SNLUniverse::getAssign();
  ASSERT_NE(nullptr, assign);
  EXPECT_TRUE(SNLUniverse::isDB0Primitive(assign));
  EXPECT_EQ(0, assign->getID());
  auto assignInput = SNLUniverse::getAssignInput();
  ASSERT_NE(nullptr, assignInput);
  EXPECT_EQ(0, assignInput->getID());
  auto assignOutput = SNLUniverse::getAssignOutput();
  ASSERT_NE(nullptr, assignOutput);
  EXPECT_EQ(1, assignOutput->getID());
  SNLBitNet* assignFT = assignInput->getNet();
  ASSERT_NE(nullptr, assignFT);
  EXPECT_EQ(assignOutput->getNet(), assignFT);

  auto db1 = SNLDB::create(SNLUniverse::get());
  EXPECT_FALSE(SNLUniverse::isDB0(db1));
}

TEST_F(SNLDB0Test, testAND) {
  EXPECT_EQ(nullptr, SNLUniverse::get());
  EXPECT_EQ(nullptr, SNLDB0::getAND(2));

  SNLUniverse::create();
  ASSERT_NE(nullptr, SNLUniverse::get());

  SNLDesign* and2 = SNLDB0::getAND(2);
  ASSERT_NE(nullptr, and2);
}