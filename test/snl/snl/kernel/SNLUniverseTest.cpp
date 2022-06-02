#include "gtest/gtest.h"

#include "SNLUniverse.h"
using namespace naja::SNL;

class SNLUniverseTest: public ::testing::Test {
  protected:
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
};

TEST_F(SNLUniverseTest, test) {
  ASSERT_EQ(nullptr, SNLUniverse::get());
  SNLUniverse::create();
  ASSERT_NE(nullptr, SNLUniverse::get());
  auto universe = SNLUniverse::get();
  ASSERT_TRUE(universe);
}