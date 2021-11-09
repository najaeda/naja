#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
using namespace SNL;

class SNLUniverseTest: public ::testing::Test {
  protected:
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
};

TEST_F(SNLUniverseTest, test) {
  ASSERT_FALSE(SNLUniverse::get());
  SNLUniverse::create();
  ASSERT_TRUE(SNLUniverse::get());
  auto universe = SNLUniverse::get();
  ASSERT_TRUE(universe);

  //dbo
  auto assign = SNLUniverse::getAssign();
  ASSERT_TRUE(assign);
  EXPECT_EQ(0, assign->getID());
  auto assignInput = SNLUniverse::getAssignInput();
  ASSERT_TRUE(assignInput);
  EXPECT_EQ(0, assignInput->getID());
  auto assignOutput = SNLUniverse::getAssignOutput();
  ASSERT_TRUE(assignOutput);
  EXPECT_EQ(1, assignOutput->getID());
}
